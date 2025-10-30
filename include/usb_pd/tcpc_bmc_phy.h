#ifndef _TCPC_BMC_PHY_H
#define _TCPC_BMC_PHY_H


struct tcpcPhyChannel {
    PIO pio;
    uint sm_tx;
    uint sm_rx;
    uint irq;
    uint pin_rx;
    uint pin_tx_high;
    uint pin_tx_low;
    uint dma_rx;
    uint32_t *raw_buf_rx;
    uint raw_buf_rx_size;
    uint process_idx_rx;
};
typedef struct tcpcPhyChannel tcpcPhyChannel;

struct tcpcBmcPhyTxData {
    uint32_t *pio_raw_tx;
    uint num_zeros;
    uint num_u32;
};
typedef struct tcpcBmcPhyTxData tcpcBmcPhyTxData;

tcpcBmcPhyTxData* tcpc_bmc_phy_tx_prepare(pd_frame *pdf);
void tcpc_bmc_phy_tx_send(tcpcPhyChannel *phy_ch, tcpcBmcPhyTxData *tx_data);

#endif

