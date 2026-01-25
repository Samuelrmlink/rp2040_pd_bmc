#include "main_i.h"

// Returns the index value of the 'Ordered Set'
uint typec_pdframe_orderedset_get_idx(uint32_t input) {
    uint idx;
    // TODO: Debug RX PD PHY - sometimes first symbol is droppped
    // ----- Workaround solution: shift bits if symbol is dropped
    if(input & 0xFF000000) {
        switch(input) {
            case(ordsetHardReset):          idx = pdfTypeHardReset; /* 1 */ break;
            case(ordsetCableReset):         idx = pdfTypeCableReset;/* 2 */ break;
            case(ordsetSop):                idx = pdfTypeSop;       /* 3 */ break;
            case(ordsetSopP):               idx = pdfTypeSopP;      /* 4 */ break;
            case(ordsetSopDp):              idx = pdfTypeSopDp;     /* 5 */ break;
            case(ordsetSopPDbg):            idx = pdfTypeSopPDbg;   /* 6 */ break;
            case(ordsetSopDpDbg):           idx = pdfTypeSopDpDbg;  /* 7 */ break;
            default:                        idx = 0;                /* 0 */
        }
    } else {
        switch(input) {
            case(ordsetHardReset >> 8):     idx = pdfTypeHardReset; /* 1 */ break;
            case(ordsetCableReset >> 8):    idx = pdfTypeCableReset;/* 2 */ break;
            case(ordsetSop >> 8):           idx = pdfTypeSop;       /* 3 */ break;
            case(ordsetSopP >> 8):          idx = pdfTypeSopP;      /* 4 */ break;
            case(ordsetSopDp >> 8):         idx = pdfTypeSopDp;     /* 5 */ break;
            case(ordsetSopPDbg >> 8):       idx = pdfTypeSopPDbg;   /* 6 */ break;
            case(ordsetSopDpDbg >> 8):      idx = pdfTypeSopDpDbg;  /* 7 */ break;
            default:                        idx = 0;                /* 0 */
        }
    }
    return idx;
}
// Returns true if valid (valid 'Ordered Set' and CRC32)
bool typec_pdframe_valid(pd_frame *pdf) {
    if(!pdf->timestamp_us) {
        return false;       // No timestamp - not valid
    }
    uint ordset_idx = typec_pdframe_orderedset_get_idx(pdf->ordered_set);
    switch(ordset_idx) {
        case(pdfTypeInvalid):
            return false;   // Invalid
        case(pdfTypeHardReset):
        case(pdfTypeCableReset):
            return true;    // Valid - Hard/Cable resets don't have a CRC32 value
        case(pdfTypeSop):
        case(pdfTypeSopP):
        case(pdfTypeSopDp):
        case(pdfTypeSopPDbg):
        case(pdfTypeSopDpDbg):
            return crc32_pdframe_valid(pdf);    // Valid if CRC32 matches
    }
}
// Returns the number of unchunked extended bytes (chunked extended frames, or non-extended frames will return 0)
uint typec_pdframe_extended_unchunked_bytes(pd_frame *pdf) {
    // Check if frame is unchunked extended
    if((pdf->hdr >> 15) && !(pdf->ext_hdr >> 15)) {
        // Frame is unchunked extended - return the number of bytes that need to be reserved for this frame.
        return pdf->ext_hdr & 0x1FF;
    } else {
        // Frame is chunked extended or non-extended
        return 0;
    }
}
void typec_pdframe_generate_goodcrc(pd_frame *input_frame, pd_frame *output_frame) {
    // Ensure we start with a clean slate
    memset(output_frame, 0, sizeof(pd_frame));
    // Transfer over frame type (SOP, SOP', SOP", etc..)
    output_frame->ordered_set = bmcFrameType[typec_pdframe_orderedset_get_idx(input_frame->ordered_set)];
    // Transfer the MsgID, Spec Rev. and apply the GoodCRC Msg Type.
    output_frame->hdr = (input_frame->hdr & 0xE00) | (input_frame->hdr & 0xC0) | (0x2 << 6) | (uint8_t)controlMsgGoodCrc;
    // Generate CRC32
    output_frame->obj[0] = crc32_pdframe_calc(output_frame);
}
PDMessageType typec_pdframe_get_sop_msg_type(pd_frame *msg) {
    uint frame_type;
    if(msg->hdr & 0x8000) { frame_type = 1u << 7;           // Extended message
    } else if(msg->hdr & 0x7000) { frame_type = 1u << 6;    // Data message
    } else { frame_type = 0; }                              // Control message
    return (PDMessageType) frame_type | (msg->hdr & 0x1F);
}
uint typec_pdframe_get_hdr_msgid(pd_frame *pdf) {
    return (pdf->hdr >> 9) & 0x7;
}
void typec_pdframe_set_hdr_msgid(pd_frame *pdf, uint msgid) {
    // Clear MsgID field
    pdf->hdr &= (pdf->hdr >> 9) & 0x7;
    // Write to MsgID field
    pdf->hdr &= msgid & 0x7;
}
void typec_pdframe_inc_msgid(pd_frame *pdf) {
    // Read MsgID
    uint val = typec_pdframe_get_hdr_msgid(pdf);
    // Increment MsgID (bit overflow will reset to zero)
    val++;
    // Write MsgID
    typec_pdframe_set_msgid(pdf, val);
}
// Returns true if pd_frame objects match
bool typec_pdframe_compare(pd_frame *pdf_a, pd_frame *pdf_b) {
    // Compare frame type (SOP, SOP', SOP", etc..)
    uint32_t ordset_a = bmcFrameType[typec_pdframe_orderedset_get_idx(pdf_a->ordered_set)];
    uint32_t ordset_b = bmcFrameType[typec_pdframe_orderedset_get_idx(pdf_b->ordered_set)];
    // This is done with the 'Ordered Set' idx value intentionally
    if(ordset_a != ordset_b) { return false; }
    // Compare the rest of the frame
    if(memcmp(&((pdf_a->raw_bytes)[8]), &((pdf_b->raw_bytes)[8]), sizeof(uint8_t) * 52) != 0) { return false; }
    // Return if we're still here
    return true;
}
