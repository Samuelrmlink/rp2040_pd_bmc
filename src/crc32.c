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
    uint8_t *tmp_ptr = &tmpval[2];

    
/* - TODO: Need to implement unchunked Extended Header CRC generation - chunked 
 * Extended Header packets work normally without this commented section. 
    // Extended Header (TODO)
    if((pdf->hdr >> 15) && !((pdf->obj[0] >> 15) & 0x1)) {
	tmpval[2] = pdf->obj[0] & 0xFF;
	tmpval[3] = (pdf->obj[0] & 0xFF00) >> 8;
	num_bytes += 2;
    }
    //
*/

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
