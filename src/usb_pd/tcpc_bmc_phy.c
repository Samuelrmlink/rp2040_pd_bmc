#include "main_i.h"

tcpcBmcPhyTxData* tcpc_bmc_phy_tx_prepare(pd_frame *pdf) {
    tcpcBmcPhyTxData *tx_data = malloc(sizeof(tcpcBmcPhyTxData));
    uint32_t *bitstream = typec_pretx_convert(pdf);
    tx_data->num_u32 = bitstream[0];
    tx_data->num_zeros = typec_pretx_num_leading_zeros(bitstream[1]);
    tx_data->pio_raw_tx = typec_tx_convert(&bitstream[1], bitstream[0]);
    //printf("%X %X %X %X %X %X %X %X %X %X %X\n", converted[0], converted[1], converted[2], converted[3], converted[4], converted[5], converted[6], converted[7], converted[8], converted[9], converted[10]);
    free(bitstream);
    return tx_data;
}
void tcpc_bmc_phy_tx_send(tcpcPhyChannel *phy_ch, tcpcBmcPhyTxData *tx_data) {
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
    printf("ZI %u %u\n", tx_data->num_zeros, tx_data->num_u32);
    //for(uint i = 0; i < 10; i++) { printf("%08X ", (tx_data->pio_raw_tx)[i]); } printf("\n");
    free(tx_data->pio_raw_tx);
}
