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
    
    // Is this frame extended unchunked? (If so - how many bytes is the payload?)
    uint8_t num_ext_bytes = typec_pdframe_extended_unchunked_bytes(pdf);
    if(num_ext_bytes) {
        // Account for the extended header + payload bytes
        num_bytes += 2 + num_ext_bytes;
    } else {
        // Account for total number of bytes occupied by any potential data objects
        num_bytes += ((pdf->hdr >> 12) & 0x7) * 4;
    }

    // Call the CRC32 generating function
    uint32_t crc_val = crc32_calc(&(pdf->raw_bytes)[10], num_bytes);
    // Return CRC
    return crc_val;
}
// Returns true if *pd_frame is valid
bool crc32_pdframe_valid(pd_frame *pdf) {
    // Determine CRC offset
    uint crc_offset =
            12                                              // Base LSB offset of obj[0] (alternatively ext_hdr) field
            + (pdf->hdr >> 12 & 0x7) * 4                    // Data Objects Size (chunked extended messages are also accounted for here)
            + typec_pdframe_unchunked_size(pdf);            // Extended Unchunked Size [ext_hdr + extended data payload]
    uint32_t *crc32_ptr = &(pdf->raw_bytes[crc_offset]);
    // Return whether CRC matches
    if(*crc32_ptr != crc32_pdframe_calc(pdf)) {
        printf("CRC fail: %X %X Frame: %X %X %X %X %X %X %X %X %X %X\n", *crc32_ptr, crc32_pdframe_calc(pdf), pdf->hdr, pdf->ext_hdr, pdf->obj[0], pdf->obj[1], pdf->obj[2], pdf->obj[3], pdf->obj[4], pdf->obj[5], pdf->obj[6], pdf->obj[7]);
    }
    return *crc32_ptr == crc32_pdframe_calc(pdf);
}
