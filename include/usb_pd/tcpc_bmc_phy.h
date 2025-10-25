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
    uint *raw_buf_rx;
    uint raw_buf_rx_size;
    uint process_idx_rx;
};
typedef struct tcpcPhyChannel tcpcPhyChannel;

#endif

