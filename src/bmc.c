#include "main_i.h"

// BMC channel pointers
bmcChannel *bmc_ch0;

bmcRx* bmc_rx_setup() {
    bmcRx *rx = malloc(sizeof(bmcRx));
    rx->rolloverObj = 12;
    rx->pdfPtr = malloc(sizeof(pd_frame) * rx->rolloverObj);
    rx->objOffset = 0;
    rx->byteOffset = 0;
    rx->upperSymbol = false;
    rx->scrapBits = 0;
    rx->afterScrapOffset = 0;
    rx->inputOffset = 0;
    rx->rx_crc32 = 0;
    rx->eval_crc32 = false;
    return rx;
}
uint32_t bmc_get_timestamp(bmcRx *rx) {
    return (rx->pdfPtr)[rx->objOffset].timestamp_us;
}
// Returns the number of unchunked extended bytes (chunked extended frames, or non-extended frames will return 0)
uint8_t bmc_extended_unchunked_bytes(pd_frame *pdf) {
    if((pdf->hdr >> 15) && !(pdf->raw_bytes[12] >> 7)) {
        return (pdf->raw_bytes[12] | (pdf->raw_bytes[12] & 0x1) << 8);
    } else {
        return 0;
    }
}

void bmc_locate_sof(bmcRx *rx, uint32_t *in) {
    // While loop ONLY runs if there are at least 5 bits in the input buffer
    while(rx->inputOffset <= 27) {
        if( ((rx->scrapBits | (*in >> rx->inputOffset) << rx->afterScrapOffset) & 0x1F) == symKcodeS1 ||     // K-Code S1 (starting symbol for ordsetSop*) ---or---
            ((rx->scrapBits | (*in >> rx->inputOffset) << rx->afterScrapOffset) & 0x1F) == symKcodeR1) {     // K-code R1 (starting symbol for ordsetHardReset, ordsetCableReset)
            // Save the timestamp
            (rx->pdfPtr)[rx->objOffset].timestamp_us = time_us_32();
            // Set Ordered Set starting offset
            rx->byteOffset = 4;
            // Exit - we've found the Start-of-frame
            break;
        } else {
            // If we have scraps leftover we'll consume those first
            if(rx->afterScrapOffset) {
                rx->afterScrapOffset -= 1;
                rx->scrapBits >>= 1;
            } else {
                // Try offsetting another bit
                rx->inputOffset += 1;
            }
        }
    }
    if(rx->inputOffset > 27) {
        rx->scrapBits = *in >> rx->inputOffset;
        rx->afterScrapOffset = 32 - rx->inputOffset;
    }
}
// Returns 4 bits (combined from both scrap and pio_raw)
uint8_t bmc_pull_5b(bmcRx *rx, uint32_t *pio_raw) {
    uint8_t decode_4b = bmc5bTo4b[(rx->scrapBits | (*pio_raw >> rx->inputOffset) << rx->afterScrapOffset) & 0x1F];
    // Increment offset to account for bits consumed from pio_raw
    rx->inputOffset += 5 - rx->afterScrapOffset;
    // We can reset to zero since the scrapBits variable never holds more than 4 bits
    rx->scrapBits = 0;
    rx->afterScrapOffset = 0;
    return decode_4b;
}
bool bmc_load_symbols(bmcRx *rx, uint32_t *pio_raw) {
    // While loop ONLY runs if there are at least 5 bits in the input buffer
    uint8_t decode_4b;
    while(rx->inputOffset <= 27) {
        decode_4b = bmc_pull_5b(rx, pio_raw);
        if(decode_4b == sym4bKcodeEop) {
            // End of packet symbol
            return true;
        }
        // Skip past padding (put there for ARM Cortex-M alignment purposes)
        if(rx->byteOffset == 8) {
            // Skip to hdr field offset
            rx->byteOffset = 10;
        }
        // Transfer symbol
        (rx->pdfPtr)[rx->objOffset].raw_bytes[rx->byteOffset] |= decode_4b << (4 * rx->upperSymbol);
        if(rx->upperSymbol || decode_4b & 0x10) {
            rx->upperSymbol = false;
            rx->byteOffset++;
        } else {
            rx->upperSymbol = true;
        }
    }
    if(rx->inputOffset > 27) {
        rx->scrapBits = *pio_raw >> rx->inputOffset;
        rx->afterScrapOffset = 32 - rx->inputOffset;
    }
    return false;
}
void bmc_process_symbols(bmcRx *rx, uint32_t *pio_raw) {
    // Reset inputOffset (since we are iterating through a new input buffer)
    rx->inputOffset = 0;
    // Check for no timestamp (which would mean that we haven't found the end of the frame preamble)
    if(!bmc_get_timestamp(rx)) {    // No timestamp would mean that we are still in the preamble stage
        bmc_locate_sof(rx, pio_raw);
    }
    // Check again for a timestamp (again - this would mean we are past the preamble)
    if(bmc_get_timestamp(rx)) {
        if(bmc_load_symbols(rx, pio_raw)) {
            // End of frame
            rx->objOffset++;
            rx->upperSymbol = false;
//            printf("EOF\n");
        }
    }
}
void bmc_rx_check() {
    extern bmcRx *pdq_rx;
    extern bmcChannel *bmc_ch0;
    uint32_t buf;
    while(!pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
        buf = pio_sm_get(bmc_ch0->pio, bmc_ch0->sm_rx);
        bmc_process_symbols(pdq_rx, &buf);
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
// TODO: find a new place for this function
void individual_pin_toggle(uint8_t pin_num) {
    if(gpio_get(pin_num))
	gpio_clr_mask(1 << pin_num); // Drive pin low
    else
	gpio_set_mask(1 << pin_num); // Drive pin high
}
