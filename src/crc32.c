uint32_t crc32_calc(uint8_t *data, int length) {
    uint32_t crc = crc32_initial_value;
    for(int i = 0; i < length; i++) {
	crc = crc32_lookup[(crc ^ data[i])        & 0x0F] ^ (crc >> 4);
	crc = crc32_lookup[(crc ^ (data[i] >> 4)) & 0x0F] ^ (crc >> 4);
    }
    return ~crc;
}
bool crc32_isValid(uint8_t *data, int length) {
    uint32_t crc = crc32_calc(data, length);
    return (bool)(crc == crc32_residual);
}
uint32_t crc32_pdmsg(uint8_t *pd_data) {
    //PD Header
    uint16_t hdr = pd_data[0] | pd_data[1] << 8;
    uint8_t tmpval[32] = { hdr & 0xFF, hdr >> 8 };

    return crc32_calc(&tmpval, 2);
}
