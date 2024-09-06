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
uint32_t bmc_get_timestamp(bmcRx *rx) {
    return (rx->pdf_ptr)[rx->objOffset]->timestamp_us;
}
void bmc_locate_sof(bmcRx *rx, uint32_t in) {
    // Loop for a bit to search for the first K-code
    for(uint8_t i = 0; i < 80; i++) {

        if( ((((rx->pdfPtr)[rx->objOffset]->scrapBits | in << (rx->pdfPtr)[rx->objOffset]->afterScrapOffset)) & 0x1F) == symKcodeS1 ||     // K-Code S1 (starting symbol for ordsetSop*) ---or---
            ((((rx->pdfPtr)[rx->objOffset]->scrapBits | in << (rx->pdfPtr)[rx->objOffset]->afterScrapOffset)) & 0x1F) == symKcodeR1) {     // K-code R1 (starting symbol for ordsetHardReset, ordsetCableReset)
            // Save the timestamp
            (rx->pdfPtr)[rx->objOffset]->timestamp_us = bmc_get_timestamp(rx);
        } else {
            // If we have scraps leftover we'll consume those first
            if((rx->pdfPtr)[rx->objOffset]->afterScrapOffset) {
                (rx->pdfPtr)[rx->objOffset]->afterScrapOffset -= 1;
                (rx->pdfPtr)[rx->objOffset]->scrapBits >>= 1;
            } else {
                // We have no scrap bits
                (rx->pdfPtr)[rx->objOffset]->inputOffset += 1;
            }
        }
    }
    
}
void bmc_process_symbols(bmcRx *rx, uint32_t pio_raw) {
    if(!bmc_get_timestamp) {    // No timestamp would mean that we are still in the preamble stage
        
    }
}
void bmc_rx_check() {
    extern bmcRx pdq_rx;
    uint32_t buf;
    if(!pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
        buf = pio_sm_get(bmc_ch0->pio, bmc_ch0->sm_rx);
        bmcProcessSymbols(&pdq_rx, buf);
        //(pdq_rx->pdfPtr[pdq_rx->objOffset])->raw_bytes[pdq_rx->byteOffset] |= pio_sm_get(bmc_ch0->pio, bmc_ch0->sm_rx) << pdq_rx->evenSymbol;
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
