#include "main_i.h"

// BMC channel pointers
bmcChannel *bmc_ch0;

void bmc_rx_check() {
    rx_data data;
    extern QueueHandle_t queue_rx_pio;
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
    extern QueueHandle_t queue_rx_pio;
    extern QueueHandle_t queue_rx_validFrame;
    extern QueueHandle_t queue_policy;
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
