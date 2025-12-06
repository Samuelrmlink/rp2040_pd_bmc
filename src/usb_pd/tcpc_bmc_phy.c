#include "main_i.h"

#define NUM_BITS_SYMBOL 5

static uint typec_pretx_unchunked_bytes(pd_frame *pdf) {
    return (pdf->hdr >> 15) ? 2 + typec_pdframe_extended_unchunked_bytes(pdf) : 0;
}
static void typec_pretx_buf_write(uint32_t input_bits, uint num_input_bits, uint32_t *out_buf, uint *out_buf_pos) {
    uint input_lshift = 0;
    // Loop while there is data to transfer
    while(num_input_bits) {
        // Determine how many bits we can transfer this round
        uint out_obj_empty_bits = 32 - (*out_buf_pos % 32);
        uint num_transfer_bits = num_input_bits <= out_obj_empty_bits ? num_input_bits : out_obj_empty_bits;
        // Transfer bits
        out_buf[*out_buf_pos / 32] |= ((input_bits >> input_lshift) & (0xFFFFFFFF >> (32 - num_transfer_bits))) << (*out_buf_pos % 32);
        *out_buf_pos += num_transfer_bits;
        num_input_bits -= num_transfer_bits;
        input_lshift += num_transfer_bits;
    }
}
static uint typec_pretx_num_leading_zeros(uint32_t obj) {
    for(uint i = 0; i < 32; i++) {
        if(obj & (1u << i)) { return i - 1; }
    }
    return 0;
}
// Convert pd_frame to raw bitstream
static uint32_t* typec_pretx_convert(pd_frame *pdf) {
    // Calculate payload bytes (excluding K-Code symbols)
    uint num_payload_bytes =
        2 +                                 // Header
        (((pdf->hdr >> 12) & 0x7) * 4) +    // Data Objects
        typec_pretx_unchunked_bytes(pdf) +  // Ext Header/Payload
        4;                                  // CRC32
    // Calculate the number of output bits required
    uint16_t total_bits_req =
        64 + 5 * (              // Preamble + [5 bits per symbol]
        4 +                     // Ordered Set symbols
        2 * num_payload_bytes + // Payload (2 symbols per byte)
        1);                     // EOP symbol
    // Determine number of leading zeros
    uint num_zeros = (total_bits_req % 32) ? (32 - total_bits_req % 32) : 0; // Zero if no remainder
    // Determine number of u32 objects requires
    uint num_u32 = num_zeros ? total_bits_req / 32 + 1 : total_bits_req / 32;
    // Allocate u32 objects + 1 extra
    uint32_t *out = pvPortMalloc(sizeof(uint32_t) * (num_u32 + 1));
    ASSERT_MALLOC(out);
    // The first u32 object will store the allocation size
    out[0] = (uint32_t)num_u32;
    // Clear output buffer allocation
    memset(&out[1], 0, sizeof(uint32_t) * num_u32);
    // Offset for leading zeros
    uint out_bit_pos = num_zeros;
    // Add preamble (64-bits)
    typec_pretx_buf_write(0xAAAAAAAA, 32, &out[1], &out_bit_pos);
    typec_pretx_buf_write(0xAAAAAAAA, 32, &out[1], &out_bit_pos);
    // Add the Ordered Set symbols
    uint input_byte_offset = 4;
    for(uint i = 0; i < 4; i++) {
        typec_pretx_buf_write(bmc4bTo5b[pdf->raw_bytes[input_byte_offset]], (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
        input_byte_offset++;
    }
    // Add the Payload (non K-Code symbols)
    for(uint i = 0; i < num_payload_bytes; i++) {
        // Skip the __padding1 field in the pd_frame structure
        if(input_byte_offset == 8) { input_byte_offset = 10; }
        // Write both the upper and lower symbols (With the exception of K-Code symbols - each byte is represented by 2 symbols)
        typec_pretx_buf_write(bmc4bTo5b[(pdf->raw_bytes)[input_byte_offset] & 0xF], (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
        typec_pretx_buf_write(bmc4bTo5b[((pdf->raw_bytes)[input_byte_offset] >> 4) & 0xF], (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
        input_byte_offset++;
    }
    // Add the EOP symbol
    typec_pretx_buf_write(symKcodeEop, (uint)NUM_BITS_SYMBOL, &out[1], &out_bit_pos);
    return out;
}
static uint32_t* typec_tx_convert(uint32_t *in, uint num_in_obj) {
    // We should have twice are many bits at the output
    uint num_out_obj = 2 * num_in_obj;
    uint32_t *out = pvPortMalloc(sizeof(uint32_t) * (num_out_obj + 1));
    ASSERT_MALLOC(out);
    memset(out, 0, sizeof(uint32_t) * num_out_obj);
    for(uint i = 0; i < num_out_obj; i += 2) {
        uint input_obj = i / 2;
        out[i] = 0xAAAAAAAA |
                ((in[input_obj] >> 15) & 1) << 30 |
                ((in[input_obj] >> 14) & 1) << 28 |
                ((in[input_obj] >> 13) & 1) << 26 |
                ((in[input_obj] >> 12) & 1) << 24 |
                ((in[input_obj] >> 11) & 1) << 22 |
                ((in[input_obj] >> 10) & 1) << 20 |
                ((in[input_obj] >> 9) & 1) << 18 |
                ((in[input_obj] >> 8) & 1) << 16 |
                ((in[input_obj] >> 7) & 1) << 14 |
                ((in[input_obj] >> 6) & 1) << 12 |
                ((in[input_obj] >> 5) & 1) << 10 |
                ((in[input_obj] >> 4) & 1) << 8 |
                ((in[input_obj] >> 3) & 1) << 6 |
                ((in[input_obj] >> 2) & 1) << 4 |
                ((in[input_obj] >> 1) & 1) << 2 |
                ((in[input_obj] >> 0) & 1) << 0;
        out[i + 1] = 0xAAAAAAAA |
                ((in[input_obj] >> 31) & 1) << 30 |
                ((in[input_obj] >> 30) & 1) << 28 |
                ((in[input_obj] >> 29) & 1) << 26 |
                ((in[input_obj] >> 28) & 1) << 24 |
                ((in[input_obj] >> 27) & 1) << 22 |
                ((in[input_obj] >> 26) & 1) << 20 |
                ((in[input_obj] >> 25) & 1) << 18 |
                ((in[input_obj] >> 24) & 1) << 16 |
                ((in[input_obj] >> 23) & 1) << 14 |
                ((in[input_obj] >> 22) & 1) << 12 |
                ((in[input_obj] >> 21) & 1) << 10 |
                ((in[input_obj] >> 20) & 1) << 8 |
                ((in[input_obj] >> 19) & 1) << 6 |
                ((in[input_obj] >> 18) & 1) << 4 |
                ((in[input_obj] >> 17) & 1) << 2 |
                ((in[input_obj] >> 16) & 1) << 0;
    }
    out[num_out_obj - 1] ^= (1 << 31);
    return out;
}
tcpcBmcPhyTxData* tcpc_bmc_phy_tx_prepare(pd_frame *pdf) {
    tcpcBmcPhyTxData *tx_data = pvPortMalloc(sizeof(tcpcBmcPhyTxData));
    ASSERT_MALLOC(tx_data);
    uint32_t *bitstream = typec_pretx_convert(pdf);
    tx_data->num_u32 = bitstream[0];
    tx_data->num_zeros = typec_pretx_num_leading_zeros(bitstream[1]);
    tx_data->pio_raw_tx = typec_tx_convert(&bitstream[1], bitstream[0]);
    //cli_log(ERROR_LOG, "%X %X %X %X %X %X %X %X %X %X %X\n", converted[0], converted[1], converted[2], converted[3], converted[4], converted[5], converted[6], converted[7], converted[8], converted[9], converted[10]);
    vPortFree(bitstream);
    return tx_data;
}
static void tcpc_bmc_phy_wait_for_inactive_tx_sm(PIO pio, uint sm) {
    while(!pio_sm_is_tx_fifo_empty(pio, sm)) {
        busy_wait_us(1);
    }
    // Delay - 85us total
    //      60us (just to be sure that the state machine's OSR is empty)
    //    + 25us ('tInterFrameGap' from the USB Power Delivery 3.1 Specification)
    sleep_us(85);
}
void tcpc_bmc_phy_tx_send(tcpcPhyChannel *phy_ch, tcpcBmcPhyTxData *tx_data) {
    tcpc_bmc_phy_wait_for_inactive_tx_sm(phy_ch->pio, phy_ch->sm_tx);
    taskENTER_CRITICAL();
    pio_sm_put_blocking(phy_ch->pio, phy_ch->sm_tx, (tx_data->pio_raw_tx)[0]);
    for(uint i = 0; i < tx_data->num_zeros; i++) { pio_sm_exec(phy_ch->pio, phy_ch->sm_tx, pio_encode_out(pio_null, 2)); }
    pio_sm_exec(phy_ch->pio, phy_ch->sm_tx, pio_encode_set(pio_y, 1));
    pio_sm_exec(phy_ch->pio, phy_ch->sm_tx, pio_encode_mov(pio_pins, pio_y));
    busy_wait_us(1);
    pio_sm_set_enabled(phy_ch->pio, phy_ch->sm_tx, true);
    for(uint i = 1; i < (tx_data->num_u32 * 2); i++) {
        pio_sm_put_blocking(phy_ch->pio, phy_ch->sm_tx, (tx_data->pio_raw_tx)[i]);
    }
    taskEXIT_CRITICAL();
    //cli_log(ERROR_LOG, "ZI %u %u\n", tx_data->num_zeros, tx_data->num_u32);
    vPortFree(tx_data->pio_raw_tx);
}
