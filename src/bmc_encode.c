#include "main_i.h"
/*
void tx_msg_inc(uint8_t *msgId) { // Input here is actually only 3 bits - therefore ensure we rollover back to zero following 0x7
  if(*msgId < 0x7) { (*msgId)++; }
  else { *msgId = 0; }
}
void pdf_generate_goodcrc(pd_frame *input_frame, txFrame *tx) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Apply the correct frametype (SOP = 3, SOP' = 4, etc...) - don't transfer the CRC okay bit
    tx->pdf->frametype = input_frame->frametype & PDF_TYPE_MASK;

    // Transfer the MsgID, Spec Rev. and apply the GoodCRC Msg Type.
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    tx->pdf->hdr = (input_frame->hdr & 0xE00) | (input_frame->hdr & 0xC0) | (0x2 << 6) | (uint8_t)controlMsgGoodCrc;

    // Generate CRC32
    tx->crc = crc32_pdframe_calc(tx->pdf);
}
// TODO - move into a "Policy Engine"
void pdf_request_from_srccap(pd_frame *input_frame, txFrame *tx, uint8_t req_pdo, pdo_accept_criteria req) {
    switch((input_frame->obj[req_pdo - 1] >> 30) & 0x3) {
	case (pdoTypeFixed) :
	    pdf_request_from_srccap_fixed(input_frame, tx, req_pdo, req);
	    break;
	case (pdoTypeAugmented) :
	    pdf_request_from_srccap_augmented(input_frame, tx, req_pdo, req);
	    break;
	case (pdoTypeBattery) :
	case (pdoTypeVariable) :
	    break;
	default :
	    //TODO - Implement error handling
	    break;
    }
}
void pdf_request_from_srccap_fixed(pd_frame *input_frame, txFrame *tx, uint8_t req_pdo, pdo_accept_criteria req) {
  // Ensure we start with a clean slate
  pd_frame_clear(tx->pdf);

  // Get PDO maximum current
  uint16_t mA_max = (input_frame->obj[req_pdo - 1] & 0x3FF) * 10;

  // Replace maximum current value if requested value is lower
  if(req.mA_max < mA_max) {
	mA_max = req.mA_max;
  }


  // Setup frametype (SOP) and header (uses hard-coded values for testing currently)
  tx->pdf->frametype = PdfTypeSop;
  tx->msgIdOut = 0;//TODO-fix this mess
  tx->pdf->hdr = (0x1 << 12) | (tx->msgIdOut << 9) | (0x2 << 6) | 0x2; // TODO - remove magic numbers

  // Setup RDO
  tx->pdf->obj[0] =	(req_pdo << 28) |			// Object position
			(((mA_max / 10) & 0x3FF) << 10) |	// Operating current
			((mA_max / 10) & 0x3FF);				// Max current
  
  // Generate CRC32
  tx->crc = crc32_pdframe_calc(tx->pdf);
}
void pdf_request_from_srccap_augmented(pd_frame *input_frame, txFrame *tx, uint8_t req_pdo, pdo_accept_criteria req) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Since eval_pdo_augmented has passed we know the req_pdo.mV_max is within range of this VPDO
    // Get PDO max current
    uint32_t pdo_mA_max = (input_frame->obj[req_pdo - 1] & 0x7F) * 50;
    
    // Any modification of the current value does not leave this function
    if(req.mA_max > pdo_mA_max) {
	// Current requested is higher than provided by the charger - just take what we can get
	req.mA_max = pdo_mA_max;
    }
  
    // Setup frametype (SOP) and header (uses hard-coded values for testing currently)
    tx->pdf->frametype = PdfTypeSop;
    tx->msgIdOut = 0;//TODO-fix this mess
    tx->pdf->hdr = (0x1 << 12) | (tx->msgIdOut << 9) | (0x2 << 6) | 0x2; // TODO - remove magic numbers

    // Setup RDO
    tx->pdf->obj[0] =	(req_pdo << 28) |			// Object position
			(((req.mV_max / 20) & 0xFFF) << 9) |	// Output voltage
			((req.mA_max / 50) & 0x7F);		// Operating current
  
    // Generate CRC32
    tx->crc = crc32_pdframe_calc(tx->pdf);
}
void pdf_generate_source_capabilities_basic(pd_frame *input_frame, txFrame *tx) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Setup frametype (SOP) and header (uses hard-coded values for testing currently)
    tx->pdf->frametype = PdfTypeSop;
    tx->pdf->hdr = 0x61A1;

    // Add Power Data objects
    tx->pdf->obj[0] = 0x0801912C;
    tx->pdf->obj[1] = 0x0002D12C;
    tx->pdf->obj[2] = 0x0003C0FA;
    tx->pdf->obj[3] = 0x0004B0C8;
    tx->pdf->obj[4] = 0xC8DC213C;
    tx->pdf->obj[5] = 0xC9402128;

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
    uint8_t follower_zero_bits = 2;
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
    uint16_t ordered_set_startbit = 32 * txf->num_u32 - data_bits_req - follower_zero_bits;
      
//  // Ensure we are an even number of bits from the Ordered Set
//  if((ordered_set_startbit - current_bit_num) % 2) { current_bit_num++; }
      
    // Loop - write preamble into buffer
    current_bit_num = txf->num_zeros = ordered_set_startbit - 64;
    while(true) {
      tx_raw_buf_write(TX_VALUE_PREAMBLE_ADVANCE, NUM_BITS_PREAMBLE_ADVANCE, txf->out, &current_bit_num);
      // Break out of loop when we hit the first bit of the Ordered Set
      //printf("%X\n", ordered_set_startbit);
      if(current_bit_num == ordered_set_startbit) { break; }
    }

    // Write Ordered Set into buffer
    tx_raw_buf_write(bmcFrameType[txf->pdf->frametype & PDF_TYPE_MASK], (uint8_t)NUM_BITS_ORDERED_SET, txf->out, &current_bit_num);
    // Frametype is invalid, Hard Reset, or Soft Reset (not SOP, SOP', SOP\", etc..)
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
    for(int i = 0; i < ((txf->pdf->hdr >> 12) & 0x7); i++) {
      for(int x = 0; x < 8; x++) {
        tx_raw_buf_write(bmc4bTo5b[(txf->pdf->obj[i] >> (x * 4)) & 0xF], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
      }
    }

    // CRC32
    for(int i = 0; i < 8; i++) {
      tx_raw_buf_write(bmc4bTo5b[(txf->crc >> (i * 4)) & 0xF], (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);
    }

    // EOP
    tx_raw_buf_write(symKcodeEop, (uint8_t)NUM_BITS_SYMBOL, txf->out, &current_bit_num);

    // Following zero bits (if applicable)
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
void pdf_transmit(txFrame *txf, bmcChannel *ch) {
    pdf_to_uint32(txf);
    while(bmc_rx_active(ch)) {
      sleep_us(20);
    }
    irq_set_enabled(ch->irq, false);
    gpio_set_mask(1 << ch->tx_high);
    uint64_t timestamp = time_us_64();
    pio_sm_put_blocking(ch->pio, ch->sm_tx, txf->out[0]);
    pio_sm_exec(ch->pio, ch->sm_tx, pio_encode_out(pio_null, txf->num_zeros));
    for(int i = 1; i < txf->num_u32; i++) {
	pio_sm_put_blocking(ch->pio, ch->sm_tx, txf->out[i]);
    }
    busy_wait_until(timestamp + 103 * txf->num_u32 - (320 * txf->num_zeros) / 100);
    if(pio_sm_get_pc(ch->pio, ch->sm_tx) == 27) { // THIS IS A HACK - 27 is the PIO instruction that (at the time of this hack) leaves the tx line pulled high
      pio_sm_exec(ch->pio, ch->sm_tx, pio_encode_jmp(22) | pio_encode_sideset(1, 1));
    }
    gpio_clr_mask(1 << ch->tx_high);
    irq_set_enabled(ch->irq, true);
}
*/
