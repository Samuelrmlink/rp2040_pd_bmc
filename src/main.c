/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "main_i.h"

// Global BMC channel pointers
bmcChannel *bmc_ch0;

// Queues
QueueHandle_t queue_rx_pio = NULL;		// PD Frame:	Raw PIO output pipeline (1st stage)
QueueHandle_t queue_rx_validFrame = NULL;	// PD Frame:	Valid USB-PD frame	(2nd stage)
QueueHandle_t queue_policy = NULL;		// PD Frame:	Input to policy thread	(3rd stage)

// Task Handles (Thread Handles)
TaskHandle_t tskhdl_pd_rxf = NULL;	// Task handle: RX frame receiver
TaskHandle_t tskhdl_pd_pol = NULL;	// Task handle: USB-PD policy


void bmc_rx_check() {
    rx_data data;
    // If PIO RX buffer is not empty
    if(!pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
	// Receive BMC data from PIO interface
	data.val = pio_sm_get(bmc_ch0->pio, bmc_ch0->sm_rx);
	data.time = time_us_32();
	xQueueSendToBackFromISR(queue_rx_pio, (void *) &data, NULL);
    }
}
void bmc_rx_cb() {
    bmc_rx_check();
    if(pio_interrupt_get(bmc_ch0->pio, 0)) {
	pio_interrupt_clear(bmc_ch0->pio, 0);
    } 
}
bmcChannel* bmc_channel0_init() {
    bmcChannel *ch = malloc(sizeof(bmcChannel));

    // Define PIO & state machine handles
    ch->pio = pio0;
    ch->sm_tx = 0;
    ch->sm_rx = 1;

    // Define pins
    ch->rx = 6;
    ch->tx_high = 10;
    ch->tx_low = 9;
    ch->adc = 26;

    // Define IRQ channel
    ch->irq = PIO0_IRQ_0;

    // Init GPIO (not including those for PIO)
    gpio_init(ch->tx_high);
    gpio_set_dir(ch->tx_high, GPIO_OUT);
    
    // Initialize TX FIFO
    uint offset_tx = pio_add_program(ch->pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    differential_manchester_tx_program_init(ch->pio, ch->sm_tx, offset_tx, ch->tx_low, 125.f / 5);
    //pio_sm_set_enabled(ch->pio, ch->sm_tx, true);
    
    // Initialize RX FIFO
    uint offset_rx = pio_add_program(ch->pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(ch->pio, ch->sm_rx, offset_rx, ch->rx, 125.f / 5);

    // Initialize RX IRQ handler
    pio_set_irq0_source_enabled(ch->pio, pis_interrupt0, true);
    irq_set_exclusive_handler(ch->irq, bmc_rx_cb);
    irq_set_enabled(ch->irq, true);

    return ch;
}

void individual_pin_toggle(uint8_t pin_num) {
    if(gpio_get(pin_num))
	gpio_clr_mask(1 << pin_num); // Drive pin low
    else
	gpio_set_mask(1 << pin_num); // Drive pin high
}
/*
 *	USB-PD PIO data -> pd_frame data structure (as defined in pdb_msg.h header file)
 *
 */
void thread_rx_process(void* unused_arg) {
    uint32_t rxval;
    bmcDecode *bmc_d = malloc(sizeof(bmcDecode));
    bmc_d->msg = malloc(sizeof(pd_frame));
    pd_frame *rxdPdf = NULL;

    // Clear variables
    bmc_decode_clear(bmc_d);
    pd_frame_clear(bmc_d->msg);

    while(true) {
	// Block this thread if we are in the awaiting preamble stage
	if(!bmc_d->procStage) {
	    // Await new data from the BMC PIO ISR (blocking function)
	    xQueueReceive(queue_rx_pio, &bmc_d->pioData, portMAX_DELAY);
	// Otherwise check the queue without blocking (provide some time)
	} else {
	    if(!xQueueReceive(queue_rx_pio, &bmc_d->pioData, 0)) {
		sleep_us(120);
		if(!xQueueReceive(queue_rx_pio, &bmc_d->pioData, 0)) {
		    // TODO - add PIO flush function here
		    irq_set_enabled(bmc_ch0->irq, false);
		    while(pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
			pio_sm_exec_wait_blocking(bmc_ch0->pio, bmc_ch0->sm_rx, pio_encode_in(pio_y, 1));
		    }
		    irq_set_enabled(bmc_ch0->irq, true);
		    bmc_rx_check();
		    continue;
		}
	    }
	}
        bmcProcessSymbols(bmc_d, queue_rx_validFrame);
        
	// Check for complete pd_frame data
	if(xQueueReceive(queue_rx_validFrame, &rxdPdf, 0) && is_crc_good(rxdPdf)) { // If rxd && CRC is valid
	    xQueueSendToBack(queue_policy, rxdPdf, portMAX_DELAY);
	    // Free memory at pointer to avoid memory leak
	    free(rxdPdf);
	}
    }
}
PDMessageType pdf_get_sop_msg_type(pd_frame *msg) {
    uint8_t msgType = 0;
    if(msg->hdr & 0x8000) {		// Extended message
	msgType = 1 << 7;
    } else if(msg->hdr & 0x7000) {	// Data message
	msgType = 1 << 6;
    } else {				// Control messsage
	msgType = 0;
    }
    msgType |= msg->hdr & 0x1f;
    return (PDMessageType) msgType;
}

void thread_rx_policy(void *unused_arg) {
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    pd_frame *cFrame = malloc(sizeof(pd_frame));
    txFrame *txf = malloc(sizeof(txFrame));
    txf->pdf = malloc(sizeof(pd_frame));
    pd_frame latestSrcCap, latestReqDataObj;
    pd_frame_clear(&latestSrcCap);
    pd_frame_clear(&latestReqDataObj);
    uint32_t timestamp_now = 0;
    PDMessageType msgType;
    bool analyzer_mode = false;

    while(true) {
	xQueueReceive(queue_policy, cFrame, portMAX_DELAY);
	timestamp_now = time_us_32();
	//printf("%u:%u - %s Header: %X %s | %X:%X:%X\n", cFrame->timestamp_us, (timestamp_now - cFrame->timestamp_us), sopFrameTypeNames[cFrame->frametype & 0x7], cFrame->hdr, pdMsgTypeNames[pdf_get_sop_msg_type(cFrame)], cFrame->obj[0], cFrame->obj[1], cFrame->obj[2]);
	if(is_crc_good(cFrame) && (pdf_get_sop_msg_type(cFrame) != controlMsgGoodCrc) && is_sop_frame(cFrame) && !analyzer_mode && (cFrame->hdr != latestSrcCap.hdr)) {
        /*
	    // Start generating a GoodCRC response frame
	    pdf_generate_goodcrc(cFrame, txf);
	    pdf_to_uint32(txf);

	    // Send the response frame to the TX thread
        */
    //memcpy(&latestSrcCap, cFrame, sizeof(pd_frame));
	printf("%u:%u - %s Header: %X %s | %X:%X:%X\n", cFrame->timestamp_us, (timestamp_now - cFrame->timestamp_us), sopFrameTypeNames[cFrame->frametype & 0x7], cFrame->hdr, pdMsgTypeNames[pdf_get_sop_msg_type(cFrame)], cFrame->obj[0], cFrame->obj[1], cFrame->obj[2]);
	}
    }
}
int main() {
    // Initialize IO & PIO
    stdio_init_all();
    bmc_ch0 = bmc_channel0_init();

    // Setup tasks
    BaseType_t status_task_rx_frame = xTaskCreate(thread_rx_process, "PROC_THREAD", 1024, NULL, 1, &tskhdl_pd_rxf);
    BaseType_t status_task_policy = xTaskCreate(thread_rx_policy, "POLICY_THREAD", 1024, NULL, 2, &tskhdl_pd_pol);

    if(status_task_rx_frame == pdPASS) {
	// Setup the queues
	queue_rx_pio = xQueueCreate(1000, sizeof(rx_data));
	queue_rx_validFrame = xQueueCreate(10, sizeof(pd_frame));
	queue_policy = xQueueCreate(10, sizeof(pd_frame));

    // Test raw frame generation - TODO: remove
    pd_frame *cFrame = malloc(sizeof(pd_frame));
    txFrame *txf = malloc(sizeof(txFrame));
    txf->pdf = malloc(sizeof(pd_frame));
    pdf_generate_source_capabilities_basic(cFrame, txf);
    //printf("txlow: %u\n", bmc_ch0->tx_low);
    pdf_transmit(txf, bmc_ch0);
    busy_wait_us(500000);
	
	// Start the scheduler
	vTaskStartScheduler();
    } else {
	printf("Unable to start task scheduler.\n");
    }
}
