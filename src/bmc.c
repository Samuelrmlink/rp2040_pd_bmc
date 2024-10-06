#include "main_i.h"

// BMC channel pointers
bmcChannel *bmc_ch0;

// Increment object offset safely (without overflow)
void bmc_inc_object_offset(bmcRx *rx) {
    if(rx->objOffset >= rx->rolloverObj - 1) {
        // We need to rollover
        rx->objOffset = 0;
        rx->inputRollover = true;
    } else {
        // Normal incrememt operation
        rx->objOffset += 1;
    }
}
bmcRx* bmc_rx_setup() {
    bmcRx *rx = malloc(sizeof(bmcRx));
    rx->rolloverObj = 240;
    rx->pdfPtr = malloc(sizeof(pd_frame) * rx->rolloverObj);
    rx->objOffset = 0;
    rx->byteOffset = 0;
    rx->upperSymbol = false;
    rx->inputRollover = false;
    rx->scrapBits = 0;
    rx->afterScrapOffset = 0;
    rx->inputOffset = 0;
    rx->rx_crc32 = 0;
    rx->eval_crc32 = false;
    rx->overflowCount = 0;
    rx->lastOverflow = 0;
    return rx;
}
void bmc_rx_overflow_protect(bmcRx *rx) {
    rx->byteOffset = 0;
    rx->upperSymbol = 0;
    rx->scrapBits = 0;
    rx->afterScrapOffset = 0;
    rx->inputOffset = 0;
    rx->rx_crc32 = 0;
    rx->eval_crc32 = false;

    // Increment overflow count
    rx->overflowCount += 1;

    // Increment object offset
    rx->lastOverflow = rx->objOffset;
    rx->objOffset += 1;
}
uint32_t bmc_get_timestamp(bmcRx *rx) {
    return (rx->pdfPtr)[rx->objOffset].timestamp_us;
}
// Returns true if valid, otherwise returns false
bool bmc_validate_pdf(pd_frame *pdf) {
    if(!pdf->timestamp_us) {
        return false;           // No timestamp - not valid
    }
    uint32_t ordset_index = bmc_get_ordset_index(pdf->ordered_set);
    switch (ordset_index) {
        case(PdfTypeInvalid) :
            return false;       // Invalid - no valid (obviously..)
        case(PdfTypeHardReset) :
        case(PdfTypeCableReset) :
            return true;        // Valid
        case(PdfTypeSop) :
        case(PdfTypeSopP) :
        case(PdfTypeSopDp) :
        case(PdfTypeSopPDbg) :
        case(PdfTypeSopDpDbg) :
            return crc32_pdframe_valid(pdf); // Check CRC32 - valid if it checks out
    }
}
// Returns the index value of the Ordered Set
uint8_t bmc_get_ordset_index(uint32_t input) {
    uint8_t out;
    switch(input) {
        case (ordsetHardReset) :
            out = 1;
            break;
        case (ordsetCableReset) :
            out = 2;
            break;
        case (ordsetSop) :
            out = 3;
            break;
        case (ordsetSopP) :
            out = 4;
            break;
        case (ordsetSopDp) :
            out = 5;
            break;
        case (ordsetSopPDbg) :
            out = 6;
            break;
        case (ordsetSopDpDbg) :
            out = 7;
            break;
        default :
            out = 0;
    }
    return out;
}
// Returns the number of unchunked extended bytes (chunked extended frames, or non-extended frames will return 0)
uint8_t bmc_extended_unchunked_bytes(pd_frame *pdf) {
    if((pdf->hdr >> 15) && !(pdf->raw_bytes[12] >> 7)) {
        return (pdf->raw_bytes[12] | (pdf->raw_bytes[12] & 0x1) << 8);
    } else {
        return 0;
    }
}
void bmc_locate_preamble(bmcRx *rx, uint32_t *in) {
    uint8_t offset = 0;
    // This function will align with the preamble (if found)
    switch(*in) {
        case (0x55555555) :
            offset = 1;
            // Falls through (no break)
        case (0xAAAAAAAA) :
            // Indicate that the preamble has been found
            rx->byteOffset = 4;
            break;
        default:
            rx->byteOffset = 0;
            break;
    }
    // Consume all bytes (except for maybe one)
    rx->inputOffset = 32 - offset;

    rx->scrapBits = 0;
    rx->afterScrapOffset = 0;
}
void bmc_locate_sof(bmcRx *rx, uint32_t *in) {
    uint8_t num_scrap_consuming;
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
            // Consume either 1 or 2 scrap bits (depending on whether more than 1 is available)
            (rx->afterScrapOffset > 1) ? (num_scrap_consuming = 2) : (num_scrap_consuming = 1);
            // If we have scraps leftover we'll consume those first
            if(rx->afterScrapOffset) {
                rx->afterScrapOffset -= num_scrap_consuming;
                rx->scrapBits >>= num_scrap_consuming;
            }
            // Try offsetting another 2 bits
            // (Why not just offset 1 bit?: to not lose alignment from the preamble)
            rx->inputOffset += (2 - num_scrap_consuming);
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
    uint8_t ordset_index;
    // While loop ONLY runs if there are at least 5 bits in the input buffer
    uint8_t decode_4b;
    // Exit function if there aren't enough bits to process
    if(rx->inputOffset > 27) { return false; }
    // Otherwise process symbols
    while(rx->inputOffset <= 27) {
        decode_4b = bmc_pull_5b(rx, pio_raw);
        if(decode_4b == sym4bKcodeEop) {
            // End of packet symbol
            return true;
        }
        // Check frame type Skip past padding (put there for ARM Cortex-M alignment purposes)
        if(rx->byteOffset == 8) {
            // Skip past padding (put there for ARM Cortex-M alignment purposes)
            // (Skips to hdr field offset)
            rx->byteOffset = 10;
            ordset_index = bmc_get_ordset_index((rx->pdfPtr)[rx->objOffset].ordered_set);
            // Check for a Hard Reset or Cable Reset (possible EOF - without an EOP symbol)
            switch(ordset_index) {
                case (PdfTypeInvalid) :
                case (PdfTypeHardReset) :
                case (PdfTypeCableReset) :
                    // End of frame - consume all remaining symbols
                    return true;
                    break;
                default:
                    break;
            }
        }
        // Catch errors (before we have an overflow)
        if(rx->byteOffset >= 56) {
            // EOP was never received
            bmc_rx_overflow_protect(rx);
            return false;
        }
        // Transfer symbol
        (rx->pdfPtr)[rx->objOffset].raw_bytes[rx->byteOffset] |= decode_4b << (4 * rx->upperSymbol);
        if(rx->upperSymbol || decode_4b & 0x10) {
            rx->upperSymbol = false;
            rx->byteOffset += 1;
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
    // If rollover == true: Clear all memory allocated to incoming pd_frame data
    if(rx->inputRollover) {
        for(int i = 0; i < rx->rolloverObj; i++) {
            pd_frame_clear(&(rx->pdfPtr)[i]);
        }
    }
    // Check for no timestamp (which would mean that we haven't found the end of the frame preamble)
    if(!bmc_get_timestamp(rx)) {    // No timestamp would mean that we are still in the preamble stage
        if(rx->byteOffset >= 4) {
            bmc_locate_sof(rx, pio_raw);
        } else {
            bmc_locate_preamble(rx, pio_raw);
        }
    }
    // Check again for a timestamp (again - this would mean we are past the preamble)
    if(bmc_get_timestamp(rx) && (rx->inputOffset <= 27)) {
        if(bmc_load_symbols(rx, pio_raw)) {
            // End of frame - increment object offset
            bmc_inc_object_offset(rx);
            rx->upperSymbol = false;
            rx->byteOffset = 0;
        }
    }
}
void bmc_rx_check() {
    extern bmcRx *pdq_rx;
    extern bmcChannel *bmc_ch0;
    uint32_t buf;
    uint16_t count;
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
    differential_manchester_tx_program_init(ch->pio, ch->sm_tx, offset_tx, ch->tx_low, 25.f);
    pio_sm_set_enabled(ch->pio, ch->sm_tx, true);
    
    // Initialize RX FIFO
    uint offset_rx = pio_add_program(ch->pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(ch->pio, ch->sm_rx, offset_rx, ch->rx, 25.f); // 25.f for rp2040 28.2 for rp2350

    // Initialize RX IRQ handler
    pio_set_irq0_source_enabled(ch->pio, pis_interrupt0, true);
    irq_set_exclusive_handler(ch->irq, bmc_rx_cb);

    return ch;
}
// TODO: find a new place for this function
void individual_pin_toggle(uint8_t pin_num) {
    if(gpio_get(pin_num))
	gpio_clr_mask(1 << pin_num); // Drive pin low
    else
	gpio_set_mask(1 << pin_num); // Drive pin high
}

void static tx_raw_buf_write(uint32_t input_bits, uint8_t num_input_bits, uint32_t *buf, uint16_t *buf_position) {
  uint8_t obj_offset = *buf_position / 32;
  uint8_t bit_offset = *buf_position % 32;
  uint8_t obj_empty_bits = 32 - bit_offset;
  if(num_input_bits > obj_empty_bits) {
    buf[obj_offset] |= (input_bits & (0xFFFFFFFF >> (32 - obj_empty_bits))) << bit_offset;
    *buf_position += obj_empty_bits;
    // Don't write the bits to the buffer twice (remove from input variables)
    input_bits >>= obj_empty_bits;
    num_input_bits -= obj_empty_bits;
    // Update values generated from buf_position ptr
    obj_offset = *buf_position / 32;
    bit_offset = *buf_position % 32;
  }
  buf[obj_offset] |= (input_bits & (0xFFFFFFFF >> (32 - num_input_bits))) << bit_offset;
  *buf_position += num_input_bits;
}
void pdf_to_uint32(bmcTx *txf) {
    uint16_t current_bit_num = 0;
    uint8_t follower_zero_bits = 2;

    // Calculate the number of bytes we'll be transferring
    uint8_t num_bytes_payload = ((txf->pdf->hdr >> 12) & 0x7) * 4;
    if(!num_bytes_payload && (txf->pdf->hdr >> 15)) {
        num_bytes_payload += 2 + bmc_extended_unchunked_bytes(txf->pdf);
    }
    uint8_t input_bytes_non_kcode =
        2 +                 // Message Header
        num_bytes_payload + // Data Payload (may be zero - includes any data objects, extended header/data, etc if applicable)
        4;                  // CRC32
    // Calculate the number of output bits required
    uint16_t total_bits_req =
        64 + 5 * (              // Preamble + [5 bits per symbol]
        4 +                     // Ordered Set symbols
        4 +                     // Message Header (2 bytes * 2 symbols/byte = 4 symbols)
        num_bytes_payload * 2 + // Data Payload (2 symbols per byte)
        8 +                     // CRC32 (4 bytes * 2 symbols/byte = 8 symbols)
        1) +                    // EOP symbol
        follower_zero_bits;     // Zero bits following the frame
    txf->num_u32 = total_bits_req / 32;
    // Round up if more bits are required
    if(total_bits_req % 32) { txf->num_u32 += 1; }
    // Store the number of remainder unused bits (used later during transmit)
    txf->num_zeros = 32 - total_bits_req % 32;
    // Allocate memory for u32 values (PIO TX buffer)
    txf->out = malloc(sizeof(uint32_t) * txf->num_u32);
    // Ensure that our u32 area in memory is blank
    for(int i = 0; i < txf->num_u32; i++) {
        txf->out[i] = 0;
    }
    // Add preamble (64-bits)
    for(int i = 0; i < 2; i++) {
        txf->out[i] = 0xAAAAAAAA;
        current_bit_num += 32;
    }
    // Add the Ordered Set symbols
    txf->byteOffset = 4;
    for(int i = 0; i < 4; i++) {
        tx_raw_buf_write(bmc4bTo5b[(txf->pdf->raw_bytes)[txf->byteOffset]], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
        txf->byteOffset += 1;
    }
    // Add the Message Header, Data Payload, and CRC32
    for(int i = 0; i < input_bytes_non_kcode; i++) {
        // Skip the __padding1 field in the pd_frame structure
        if(txf->byteOffset == 8) { txf->byteOffset = 10; }
        // Write both the upper and lower symbols (With the exception of K-Code symbols - each byte contains 2 symbols)
        tx_raw_buf_write(bmc4bTo5b[(txf->pdf->raw_bytes)[txf->byteOffset] & 0xF], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
        tx_raw_buf_write(bmc4bTo5b[((txf->pdf->raw_bytes)[txf->byteOffset] >> 4) & 0xF], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
        txf->byteOffset += 1;
    }
    // Add the EOP symbol
    tx_raw_buf_write(symKcodeEop, (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
    // Following zero bit (if applicable)
    while(follower_zero_bits) {
        tx_raw_buf_write(0x0, 1, txf->out, &current_bit_num);
        follower_zero_bits--;
    }
}
bool bmc_rx_active(bmcChannel *chan) {
   uint prev = pio_sm_get_pc(chan->pio, chan->sm_rx);
   bool rx_line_active = false;
   for(int i = 0; i < 35; i++) {
    if(pio_sm_get_pc(chan->pio, chan->sm_rx) != prev) {
      rx_line_active = true;
      break;
    }
    busy_wait_us(1);
   }
   return rx_line_active;
}
void pdf_transmit(bmcTx *txf, bmcChannel *ch) {
    pdf_to_uint32(txf);
    while(bmc_rx_active(ch)) {
      sleep_us(20);
    }
    irq_set_enabled(ch->irq, false);
    gpio_set_mask(1 << ch->tx_high);
    uint64_t timestamp = time_us_64();
    //pio_sm_put_blocking(ch->pio, ch->sm_tx, txf->out[0]);
    //pio_sm_exec(ch->pio, ch->sm_tx, pio_encode_out(pio_null, txf->num_zeros));
    for(int i = 0; i < txf->num_u32; i++) {
	pio_sm_put_blocking(ch->pio, ch->sm_tx, txf->out[i]);
    }
    busy_wait_until(timestamp + 103 * txf->num_u32 - (320 * txf->num_zeros) / 100);
    if(pio_sm_get_pc(ch->pio, ch->sm_tx) == 27) { // THIS IS A HACK - 27 is the PIO instruction that (at the time of this hack) leaves the tx line pulled high
      pio_sm_exec(ch->pio, ch->sm_tx, pio_encode_jmp(22) | pio_encode_sideset(1, 1));
    }
    gpio_clr_mask(1 << ch->tx_high);
    free(txf->out);
    txf->byteOffset = 0;
    txf->upperSymbol = false;
    txf->scrapBits = 0;
    txf->numScrapBits = 0;
    txf->num_u32 = 0;
    while(!pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
        pio_sm_get(ch->pio, ch->sm_rx);
    }
    irq_set_enabled(ch->irq, true);
}
