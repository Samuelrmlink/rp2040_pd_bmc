#include "main_i.h"

uint32_t crc32_calc(uint8_t *data, int length) {
    uint32_t crc = crc32_initial_value;
    for(int i = 0; i < length; i++) {
	crc = crc32_lookup[(crc ^ data[i])        & 0x0F] ^ (crc >> 4);
	crc = crc32_lookup[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}
uint32_t crc32_pdframe_calc(pd_frame *pdf) {
    // Declare variables
    uint8_t num_bytes = 2;  // Frame Header = (2 bytes)
    
    // Is this frame extended unchunked? (If so - how many bytes is the payload?)
    uint8_t num_ext_bytes = bmc_extended_unchunked_bytes(pdf);
    if(num_ext_bytes) {
        // Account for the extended header + payload bytes
        num_bytes += 2 + num_ext_bytes;
    } else {
        // Account for total number of bytes occupied by any potential data objects
        num_bytes += ((pdf->hdr >> 12) & 0x7) * 4;
    }

    // Call the CRC32 generating function
    return crc32_calc(&(pdf->raw_bytes)[10], num_bytes);
}
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
/*
uint32_t crc32_pdframe_calc(pd_frame* pdf) {
    // Establish variables
    uint8_t num_bytes = 2;

    // PD Header
    //uint8_t tmpval[48] = { pdf->hdr & 0xFF, pdf->hdr >> 8 , 0x01, 0x80, 0x00, 0xFF};
    uint8_t tmpval[48] = { pdf->hdr & 0xFF, pdf->hdr >> 8};

    // Extended Header
    uint8_t num_ext_bytes = bmc_extended_unchunked_bytes(pdf);
    if(num_ext_bytes) {
        tmpval[num_bytes]	= pdf->extended_hdr & 0xFF;
        tmpval[num_bytes + 1]	= pdf->extended_hdr >> 8;
        num_bytes += 2;
    }
    for(int i = 0; i < num_ext_bytes; i++) {
        tmpval[num_bytes + i] = pdf->data[i];
    }
    num_bytes += num_ext_bytes;

    // Data Objects (if applicable)
    uint8_t num_obj = (pdf->hdr >> 12) & 0x7;//TODO: compiler definition
    for(int obj = 0; obj < num_obj; obj++) {
        for(int byte = 0; byte < 4; byte++) {
            tmpval[num_bytes + (obj * 4) + byte] = pdf->raw_bytes[12 + (obj * 4) + byte];
        }
    }

    num_bytes += num_obj * 4;

    return crc32_calc((uint8_t *) &tmpval, num_bytes);
}
*/
