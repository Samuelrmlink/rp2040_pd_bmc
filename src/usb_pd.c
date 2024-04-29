#include "main_i.h"

void bmc_decode_clear(bmcDecode* bmc_d) {
    bmc_d->procStage = 0;
    bmc_d->procSubStage = 0;
    bmc_d->pioData.val = 0;
    bmc_d->procBuf = 0;
    bmc_d->pOffset = 0;
    bmc_d->crcTmp = 0;
}
void pd_frame_clear(pd_frame *pdf) {
    for(uint8_t i = 0; i < 56; i++) {
	pdf->raw_bytes[i] = 0;
    }
}
bool is_crc_good(pd_frame *pdf) {
    if(pdf->frametype & 0x80)
	return true;
    else
	return false;
}
bool is_sop_frame(pd_frame *pdf) {
    if((pdf->frametype & 0x7) == 3)
	return true;
    else 
	return false;
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
bool is_src_cap(pd_frame *pdf) {
    if(((pdf->frametype & 0x7) == PdfTypeSop) && (pdf_get_sop_msg_type(pdf) == dataMsgSourceCap)) {
        return true;
    } else {
        return false;
    }
}
uint8_t optimal_pdo(pd_frame *pdf, uint16_t req_mvolts) {
    uint8_t ret = 0;
    // We are already assuming for this function that a valid Source Cap message is being passed in
    for(int i = 1; i <= ((pdf->hdr >> 12) & 0x7); i++) {
        if(((pdf->obj[i - 1] >> 10) & 0x3FF) * 50 == req_mvolts) {
            ret = i;
        }
    }
    return ret;
}
/*
void pdf_generate_request(pd_frame *pdf, txFrame *txf, uint8_t req_index) {
  // Ensure we start with a clean slate
  pd_frame_clear(tx->pdf);

  // Setup frametype (SOP) and header (uses hard-coded values for testing currently)
  tx->pdf->frametype = PdfTypeSop;
  tx->pdf->hdr = (0x1 << 12) | (tx->msgIdOut << 9) | (0x2 << 6) | 0x2; // TODO - remove magic numbers

  // Setup RDO
  tx->pdf->obj[0] =	(0x1 << 28) |			// Object position
			((input_frame->obj[0] & 0x3FF) << 10) |	// Operating current
			(input_frame->obj[0] & 0x3FF);		// Max current
  
  // Generate CRC32
  tx->crc = crc32_pdframe_calc(tx->pdf);
}*/