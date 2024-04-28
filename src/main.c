/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "main_i.h"

// Compiler state machine number
#define SM_TX 0
#define SM_RX 1

//Define pins (to be used by PIO for BMC TX/RX)
const uint pin_tx = 9;
const uint pin_rx = 6;

// PIO instances
PIO pio = pio0;

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
    if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
	// Receive BMC data from PIO interface
	data.val = pio_sm_get(pio, SM_RX);
	data.time = time_us_32();
	xQueueSendToBackFromISR(queue_rx_pio, (void *) &data, NULL);
    }
}
void bmc_rx_cb() {
    bmc_rx_check();
    if(pio_interrupt_get(pio, 0)) {
	pio_interrupt_clear(pio, 0);
    } 
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
		    irq_set_enabled(PIO0_IRQ_0, false);
		    while(pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
			pio_sm_exec_wait_blocking(pio, SM_RX, pio_encode_in(pio_y, 1));
		    }
		    irq_set_enabled(PIO0_IRQ_0, true);
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
/*
    memcpy(&latestSrcCap, cFrame, sizeof(pd_frame));
    sleep_us(4);

    // Test raw frame generation - TODO: remove
    pd_frame *cFrame = malloc(sizeof(pd_frame));
    txFrame *txf = malloc(sizeof(txFrame));
    txf->pdf = malloc(sizeof(pd_frame));
    cFrame->frametype = PdfTypeSop;
    pdf_generate_goodcrc(cFrame, txf);
    //txf->pdf->frametype = PdfTypeHardReset;
    pdf_to_uint32(txf);
    txf->out[0] |= 0x1;
    //printf("Pdf%u Hdr: %X Crc:%X\n", txf->pdf->frametype & PDF_TYPE_MASK, txf->pdf->hdr, txf->crc);
    pio_sm_set_enabled(pio, SM_TX, true);
    irq_set_enabled(PIO0_IRQ_0, false);
    gpio_set_mask(1 << 10);
    busy_wait_us(3);
    for(int i = 0; i < txf->num_u32; i++) {
        pio_sm_put_blocking(pio, SM_TX, txf->out[i]);
    }
    free(txf->out);
    busy_wait_us(110 * txf->num_u32);
    gpio_clr_mask(1 << 10);
    irq_set_enabled(PIO0_IRQ_0, true);
    sleep_us(1000);



    txf->msgIdOut = 0;
    pdf_request_from_srccap(&latestSrcCap, txf, 1);
	pdf_to_uint32(txf);
    //txf->out[0] |= 0x1;
	irq_set_enabled(PIO0_IRQ_0, false);
	gpio_set_mask(1 << 10);
	busy_wait_us(3);
	for(int i = 0; i < txf->num_u32; i++) {
	    pio_sm_put_blocking(pio, SM_TX, txf->out[i]);
	}
	free(txf->out);
	busy_wait_us(110 * txf->num_u32);
	gpio_clr_mask(1 << 10);
	irq_set_enabled(PIO0_IRQ_0, true);
*/
	printf("%u:%u - %s Header: %X %s | %X:%X:%X\n", cFrame->timestamp_us, (timestamp_now - cFrame->timestamp_us), sopFrameTypeNames[cFrame->frametype & 0x7], cFrame->hdr, pdMsgTypeNames[pdf_get_sop_msg_type(cFrame)], cFrame->obj[0], cFrame->obj[1], cFrame->obj[2]);
	}
    }
}
int main() {
    // Initialize IO & PIO
    stdio_init_all();
    gpio_init(10 | 8);
    gpio_set_dir(10 | 8, GPIO_OUT);

    /* Initialize TX FIFO*/
    uint offset_tx = pio_add_program(pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    differential_manchester_tx_program_init(pio, SM_TX, offset_tx, pin_tx, 125.f / 5);
    pio_sm_set_enabled(pio, SM_TX, false);
    /*
    pio_sm_put_blocking(pio, SM_TX, 0);
    pio_sm_put_blocking(pio, SM_TX, 0x0ff0a55a);
    pio_sm_put_blocking(pio, SM_TX, 0x12345678);
    pio_sm_set_enabled(pio, SM_TX, true);*/
    
    /* Initialize RX FIFO */
    uint offset_rx = pio_add_program(pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(pio, SM_RX, offset_rx, pin_rx, 125.f / 5);

    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, bmc_rx_cb);
    irq_set_enabled(PIO0_IRQ_0, true);

    // Setup tasks
    BaseType_t status_task_rx_frame = xTaskCreate(thread_rx_process, "PROC_THREAD", 1024, NULL, 1, &tskhdl_pd_rxf);
    BaseType_t status_task_policy = xTaskCreate(thread_rx_policy, "POLICY_THREAD", 1024, NULL, 2, &tskhdl_pd_pol);

    if(status_task_rx_frame == pdPASS) {
	// Setup the queues
	queue_rx_pio = xQueueCreate(1000, sizeof(rx_data));
	queue_rx_validFrame = xQueueCreate(10, sizeof(pd_frame));
	queue_policy = xQueueCreate(10, sizeof(pd_frame));

/*
    pio_sm_set_enabled(pio, SM_TX, true);
    irq_set_enabled(PIO0_IRQ_0, false);
    while(true) {
        //printf("%08x\n", pio_sm_get_blocking(pio, SM_RX));
        printf("T");
        gpio_set_mask(1 << 10);
        busy_wait_us(3);
        pio_sm_put_blocking(pio, SM_TX, 0x55555555);
        pio_sm_put_blocking(pio, SM_TX, 0x55555555);//0
        pio_sm_put_blocking(pio, SM_TX, 0x2e98d8c5);//0
        pio_sm_put_blocking(pio, SM_TX, 0x5a5e4a7d);//0
        pio_sm_put_blocking(pio, SM_TX, 0x49d77bef);//0
        pio_sm_put_blocking(pio, SM_TX, 0x6bf494a5);//0
        pio_sm_set_enabled(pio, SM_TX, true);
        busy_wait_us(590);
        gpio_clr_mask(1 << 10);
        busy_wait_us(1000000);
    }
*/
    // Test raw frame generation - TODO: remove
    pd_frame *cFrame = malloc(sizeof(pd_frame));
    txFrame *txf = malloc(sizeof(txFrame));
    txf->pdf = malloc(sizeof(pd_frame));
    pdf_generate_source_capabilities_basic(cFrame, txf);
    //txf->pdf->frametype = PdfTypeHardReset;
    pdf_to_uint32(txf);
    //txf->out[0] |= 0x1;
    printf("Pdf%u Hdr: %X Crc:%X\n", txf->pdf->frametype & PDF_TYPE_MASK, txf->pdf->hdr, txf->crc);
    for(int i = 0; i < txf->num_u32; i++) {
        printf("%X\n", txf->out[i]);
    }
    pio_sm_set_enabled(pio, SM_TX, true);
    irq_set_enabled(PIO0_IRQ_0, false);
    while(true) {
    gpio_set_mask(1 << 10);
    //busy_wait_us(3);
    pio_sm_put_blocking(pio, SM_TX, txf->out[0]);
    pio_sm_exec(pio, SM_TX, pio_encode_out(pio_null, txf->num_zeros));
    for(int i = 1; i < txf->num_u32; i++) {
        pio_sm_put_blocking(pio, SM_TX, txf->out[i]);
    }
    busy_wait_us(927);
    gpio_clr_mask(1 << 10);
    busy_wait_us(500000);
    }
	
	// Start the scheduler
	vTaskStartScheduler();
    } else {
	printf("Unable to start task scheduler.\n");
    }
}
