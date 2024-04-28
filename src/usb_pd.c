#include "main_i.h"

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