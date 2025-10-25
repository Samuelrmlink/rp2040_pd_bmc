#include "main_i.h"

// Returns the index value of the 'Ordered Set'
uint typec_pdframe_orderedset_get_idx(uint32_t input) {
    uint idx;
    // TODO: Debug RX PD PHY - sometimes first symbol is droppped
    // ----- Workaround solution: shift bits if symbol is dropped
    if(input & 0xFF000000) {
        switch(input) {
            case(ordsetHardReset):  idx = 1; break;
            case(ordsetCableReset): idx = 2; break;
            case(ordsetSop):        idx = 3; break;
            case(ordsetSopP):       idx = 4; break;
            case(ordsetSopDp):      idx = 5; break;
            case(ordsetSopPDbg):    idx = 6; break;
            case(ordsetSopDpDbg):   idx = 7; break;
            default:                idx = 0;
        }
    } else {
        switch(input) {
            case(ordsetHardReset >> 8):     idx = 1; break;
            case(ordsetCableReset >> 8):    idx = 2; break;
            case(ordsetSop >> 8):           idx = 3; break;
            case(ordsetSopP >> 8):          idx = 4; break;
            case(ordsetSopDp >> 8):         idx = 5; break;
            case(ordsetSopPDbg >> 8):       idx = 6; break;
            case(ordsetSopDpDbg >> 8):      idx = 7; break;
            default:                        idx = 0;
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
    if((pdf->hdr >> 15) && !(pdf->raw_bytes[12] >> 7)) {
        return (pdf->raw_bytes[12] | (pdf->raw_bytes[12] & 0x1) << 8);
    } else {
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
uint typec_pdframe_get_msgid(pd_frame *pdf) {
    return (pdf->hdr >> 9) & 0x7;
}
void typec_pdframe_set_msgid(pd_frame *pdf, uint msgid) {
    // Clear MsgID field
    pdf->hdr &= (pdf->hdr >> 9) & 0x7;
    // Write to MsgID field
    pdf->hdr &= msgid & 0x7;
}
void typec_pdframe_inc_msgid(pd_frame *pdf, uint msgid) {
    // Read MsgID
    uint val = typec_pdframe_get_msgid(pdf);
    // Increment MsgID (bit overflow will reset to zero)
    val++;
    // Write MsgID
    typec_pd_frame_set_msgid(pdf, val);
}
