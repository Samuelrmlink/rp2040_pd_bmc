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
        uint out_obj_empty_bits = 16 - (*out_buf_pos % 16);
        uint num_transfer_bits = num_input_bits <= out_obj_empty_bits ? num_input_bits : out_obj_empty_bits;
        // Transfer bits
        //while(num_transfer_bits > input_lshift) {
        for(uint i = 0; i < num_transfer_bits; i++) {
            out_buf[*out_buf_pos / 16] |= ((input_bits >> input_lshift) & 1u) << (*out_buf_pos % 16) * 2;
            input_lshift++;
            *out_buf_pos += 1;
        }
        //*out_buf_pos += num_transfer_bits;
        num_input_bits -= num_transfer_bits;
    }
}
// Convert pd_frame to raw bitstream
static uint32_t* tx_convert(pd_frame *pdf, tcpcBmcPhyTxData *tx_data) {
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
    uint num_zeros = (total_bits_req % 16) ? (16 - total_bits_req % 16) : 0; // Zero if no remainder
    tx_data->num_zeros = num_zeros;
    // Determine number of u32 objects requires
    uint num_u32 = num_zeros ? total_bits_req / 16 + 1 : total_bits_req / 16;
    tx_data->num_u32 = num_u32;
    // Allocate u32 objects
    uint32_t *out = pvPortMalloc(sizeof(uint32_t) * num_u32);
    ASSERT_MALLOC(out);
    // Clear output buffer allocation
    memset(out, 0xAAAAAAAA, sizeof(uint32_t) * num_u32);
    // Offset for leading zeros
    uint out_bit_pos = num_zeros;
    // Add preamble (64-bits)
    typec_pretx_buf_write(0xAAAAAAAA, 32, out, &out_bit_pos);
    typec_pretx_buf_write(0xAAAAAAAA, 32, out, &out_bit_pos);
    // Add the Ordered Set symbols
    uint input_byte_offset = 4;
    for(uint i = 0; i < 4; i++) {
        typec_pretx_buf_write(bmc4bTo5b[pdf->raw_bytes[input_byte_offset]], (uint)NUM_BITS_SYMBOL, out, &out_bit_pos);
        input_byte_offset++;
    }
    // Add the Payload (non K-Code symbols)
    for(uint i = 0; i < num_payload_bytes; i++) {
        // Skip the __padding1 field in the pd_frame structure
        if(input_byte_offset == 8) { input_byte_offset = 10; }
        // Write both the upper and lower symbols (With the exception of K-Code symbols - each byte is represented by 2 symbols)
        typec_pretx_buf_write(bmc4bTo5b[(pdf->raw_bytes)[input_byte_offset] & 0xF], (uint)NUM_BITS_SYMBOL, out, &out_bit_pos);
        typec_pretx_buf_write(bmc4bTo5b[((pdf->raw_bytes)[input_byte_offset] >> 4) & 0xF], (uint)NUM_BITS_SYMBOL, out, &out_bit_pos);
        input_byte_offset++;
    }
    // Add the EOP symbol
    typec_pretx_buf_write(symKcodeEop, (uint)NUM_BITS_SYMBOL, out, &out_bit_pos);
    // Mark the end of transmission
    out[tx_data->num_u32 - 1] ^= (1 << 31);
    return out;
}
tcpcBmcPhyTxData* tcpc_bmc_phy_tx_prepare(pd_frame *pdf) {
    tcpcBmcPhyTxData *tx_data = pvPortMalloc(sizeof(tcpcBmcPhyTxData));
    ASSERT_MALLOC(tx_data);
    uint32_t *bitstream = tx_convert(pdf, tx_data);
    tx_data->pio_raw_tx = bitstream;
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
    for(uint i = 1; i < (tx_data->num_u32); i++) {
        pio_sm_put_blocking(phy_ch->pio, phy_ch->sm_tx, (tx_data->pio_raw_tx)[i]);
    }
    taskEXIT_CRITICAL();
    //cli_log(ERROR_LOG, "ZI %u %u\n", tx_data->num_zeros, tx_data->num_u32);
    vPortFree(tx_data->pio_raw_tx);
}
