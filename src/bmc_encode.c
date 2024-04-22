#include "main_i.h"

void pdf_generate_goodcrc(pd_frame *input_frame, txFrame *tx) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Apply the correct frametype (SOP = 3, SOP' = 4, etc...) - don't transfer the CRC okay bit
    tx->pdf->frametype = input_frame->frametype & PDF_TYPE_MASK;

    // Transfer the MsgID, Spec Rev. and apply the GoodCRC Msg Type.
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    tx->pdf->hdr = (input_frame->hdr & 0xE00) | (input_frame->hdr & 0xC0) | (uint8_t)controlMsgGoodCrc;

    // Generate CRC32
    tx->crc = crc32_pdframe_calc(tx->pdf);
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
void pdf_to_uint32(txFrame *txf) {
    uint16_t current_bit_num = 0;
    uint8_t num_obj = (txf->pdf->hdr >> 12) & 0x7;
    uint16_t data_bits_req = NUM_BITS_ORDERED_SET;		// SOP sequence
    // Add bits for SOP* frames (if not HardReset, CableReset, etc.)
    if(txf->pdf->frametype >= PdfTypeSop) {
      data_bits_req +=  (10 * 2)		// Header
      +(10 * 4 * num_obj)	// Data Objects (extended header is here when chunked)
      +(10 * 4)		// CRC32
      +5;			// EOP symbol
    }
    uint16_t total_bits_req = data_bits_req + 64; // 64 for preamble
    txf->num_u32 = total_bits_req / 32;
    // Round up if more bits are required
    if(total_bits_req % 32) { txf->num_u32 += 1; }

    // Allocate memory for u32 values
    txf->out = malloc(sizeof(uint32_t) * txf->num_u32);
    //printf("Total bits required: %u\nNum u32:%u\n", total_bits_req, txf->num_u32);

    // Ensure a clean slate
    for(int i = 0; i < txf->num_u32; i++) {
      txf->out[i] = 0;
    }


    // Get Ordered Set start bit
    uint16_t ordered_set_startbit = 32 * txf->num_u32 - data_bits_req;
    // Ensure we are an even number of bits from the Ordered Set
    if((ordered_set_startbit - current_bit_num) % 2) { current_bit_num++; }
    // Loop - write preamble into buffer
    while(true) {
      tx_raw_buf_write(TX_VALUE_PREAMBLE_ADVANCE, NUM_BITS_PREAMBLE_ADVANCE, txf->out, &current_bit_num);
      // Break out of loop when we hit the first bit of the Ordered Set
      //printf("%X\n", ordered_set_startbit);
      if(current_bit_num == ordered_set_startbit) { break; }
    }

    // Write Ordered Set into buffer
    tx_raw_buf_write(bmcFrameType[txf->pdf->frametype & PDF_TYPE_MASK], (uint8_t)NUM_BITS_ORDERED_SET, txf->out, &current_bit_num);
    // Frametype is invalid, Hard Reset, or Soft Reset (not SOP, SOP', SOP", etc..)
    if(txf->pdf->frametype < PdfTypeSop) {
      // EOP/CRC is not written in this case - return function
      return;
    }


    // Include Header
    for(int i = 0; i < 4; i++) {
      tx_raw_buf_write(bmc4bTo5b[(txf->pdf->hdr >> (i * 4)) & 0xF], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
    }
    // Include Extended Header (if applicable - again - only if ext hdr exists && is unchunked - otherwise ext hdr is rolled into data objects field)

    // For loop - # for i < num_data_obj (as defined in header)
    for(int i = 0; i < ((txf->pdf->hdr >> 12) & 0x7) * 8; i++) {
      tx_raw_buf_write(bmc4bTo5b[(txf->pdf->hdr >> (i * 4)) & 0xF], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
    }

    // EOP
    tx_raw_buf_write(symKcodeEop, (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
}
