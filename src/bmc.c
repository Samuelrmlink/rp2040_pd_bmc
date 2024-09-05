#include "main_i.h"

// BMC channel pointers
bmcChannel *bmc_ch0;
/*
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
*/
void bmc_rx_check() {
    extern bmcRx pdq_rx;
    if(!pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
        (pdq_rx->pdf_ptr[pdq_rx->obj_offset])->raw_bytes[pdq_rx->byte_offset] |= pio_sm_get(bmc_ch0->pio, bmc_ch0->sm_rx) << pdq_rx->even_symbol;
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
    pio_sm_set_enabled(ch->pio, ch->sm_tx, true);
    
    // Initialize RX FIFO
    uint offset_rx = pio_add_program(ch->pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(ch->pio, ch->sm_rx, offset_rx, ch->rx, 125.f / 5);

    // Initialize RX IRQ handler
    pio_set_irq0_source_enabled(ch->pio, pis_interrupt0, true);
    irq_set_exclusive_handler(ch->irq, bmc_rx_cb);
    irq_set_enabled(ch->irq, false);    // Delay starting until we are ready to start Task Scheduler

    return ch;
}

void individual_pin_toggle(uint8_t pin_num) {
    if(gpio_get(pin_num))
	gpio_clr_mask(1 << pin_num); // Drive pin low
    else
	gpio_set_mask(1 << pin_num); // Drive pin high
}
