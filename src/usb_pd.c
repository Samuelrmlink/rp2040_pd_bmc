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