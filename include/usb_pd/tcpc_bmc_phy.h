#ifndef _TCPC_BMC_PHY
#define _TCPC_BMC_PHY

struct tcpcPhyChannel {
    PIO pio;
    uint sm_rx;
    uint irq;
    uint pin_rx;
    uint dma_rx;
    uint *raw_buf_rx;
    uint raw_buf_rx_size;
    uint process_idx_rx;
};
typedef struct tcpcPhyChannel tcpcPhyChannel;

#endif

