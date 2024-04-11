#include "main_i.h"

void pdf_generate_goodcrc(pd_frame *input_frame, txFrame *tx) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Apply the correct frametype (SOP = 3, SOP' = 4, etc...) - don't transfer the CRC okay bit
    tx->pdf->frametype = input_frame->frametype & 0x7;

    // Transfer the MsgID, Spec Rev. and apply the GoodCRC Msg Type.
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    tx->pdf->hdr = (input_frame->hdr & 0xE00) | (input_frame->hdr & 0xC0) | (uint8_t)controlMsgGoodCrc;

    // Generate CRC32
    tx->crc = crc32_pdframe_calc(tx->pdf);
}
void pdf_to_uint32(txFrame *txf) {
    uint8_t num_obj = (txf->pdf->hdr >> 12) & 0x7;
    uint16_t data_bits_req = 20		// SOP sequence
		+(10 * 2)		// Header
		+(10 * 4 * num_obj)	// Data Objects (extended header is here when chunked)
    		+(10 * 4);		// CRC32
		+5;			// EOP symbol
    uint16_t total_bits_req = data_bits_req + 64; // 64 for preamble
    txf->num_u32 = total_bits_req / 32;
    // Round up if more bits are required
    if(total_bits_req % 32) { txf->num_u32 += 1; }

    // Allocate memory for u32 values
    txf->out = malloc(sizeof(uint32_t) * txf->num_u32);
    //printf("Total bits required: %u\nNum u32:%u\n", total_bits_req, txf->num_u32);

    // Start building the uint32 array
    
}
