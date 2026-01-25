#include "main_i.h"

uint32_t crc32_calc(uint8_t *data, int length) {
    uint32_t crc = crc32_initial_value;
    for(int i = 0; i < length; i++) {
	crc = crc32_lookup[(crc ^ data[i])        & 0x0F] ^ (crc >> 4);
	crc = crc32_lookup[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}
// Returns the valid CRC32 for *pd_frame
uint32_t crc32_pdframe_calc(pd_frame *pdf) {
    // Declare variables
    uint8_t num_bytes = 2;  // Frame Header = (2 bytes)
    uint crc_calc_offset;
    
    // Is this frame extended unchunked? (If so - how many bytes is the payload?)
    uint8_t num_ext_bytes = typec_pdframe_extended_unchunked_bytes(pdf);
    if(num_ext_bytes) {
        // Account for the extended header + payload bytes
        num_bytes += 2 + num_ext_bytes;
        // Ensure that both hdr and ext_hdr are computed in the checksum
        crc_calc_offset = 8;
    } else {
        // Account for total number of bytes occupied by any potential data objects
        num_bytes += ((pdf->hdr >> 12) & 0x7) * 4;
        // Copy hdr into ext_hdr field - only hdr will be included in the checksum
        pdf->ext_hdr = pdf->hdr;
        // Ensure that only ext_hdr (hdr) will be included in the checksum
        // This is done because crc32_calc requires a contiguous block of data to compute the checksum properly
        crc_calc_offset = 10;
    }

    // Call the CRC32 generating function
    uint32_t crc_val = crc32_calc(&(pdf->raw_bytes)[crc_calc_offset], num_bytes);
    // Erase ext_hdr field if we overwrote it with hdr data
    if(!num_ext_bytes) { pdf->ext_hdr = 0; }
    // Return CRC
    return crc_val;
}
// Returns true if *pd_frame is valid
bool crc32_pdframe_valid(pd_frame *pdf) {
    uint8_t crc_obj_offset;
    // Find the object offset of the CRC
    for(int i = 0; i < 11; i++) {
        if(!(pdf->obj[i + 1])) {
            crc_obj_offset = i;
            break;
        }
    }
    // Return whether CRC matches
    return pdf->obj[crc_obj_offset] == crc32_pdframe_calc(pdf);
}
