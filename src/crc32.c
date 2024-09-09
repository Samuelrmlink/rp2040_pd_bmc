#include "main_i.h"

uint32_t crc32_calc(uint8_t *data, int length) {
    uint32_t crc = crc32_initial_value;
    for(int i = 0; i < length; i++) {
	crc = crc32_lookup[(crc ^ data[i])        & 0x0F] ^ (crc >> 4);
	crc = crc32_lookup[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}
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
