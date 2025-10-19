#include "main_i.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

static int tcpc_pio_rx_init(PIO pio, uint sm_rx, uint pin_rx) {
    float clock_div = (float)clock_get_hz(clk_sys) / 5000000;
    uint sm_rx_offset = pio_add_program(pio, &differential_manchester_rx_program);
    differential_manchester_rx_program_init(pio, sm_rx, sm_rx_offset, pin_rx, clock_div);
    pio_sm_set_enabled(pio, sm_rx, true);
}

static int tcpc_dma_handler() {
    extern tcpc_dma_channel;
    // Clear DMA interrupt
    dma_hw->ints0 = 1u << tcpc_dma_channel;

    // Do something - TODO: Add actual functionality
    printf("DMA interrupt!\n");
}
//  
//  tcpc_rx_dma_init - Initializes USB-PD RX DMA channel
//
//  pio_rx_fifo     : Pointer to PIO RX FIFO
//  pio_dreq        : PIO DREQ handle
//  raw_rx_buf      : Received Data Buffer
//  buf_size        : Received Data Buffer Size
static int tcpc_rx_dma_init(void *pio_rx_fifo, uint pio_dreq, void *raw_rx_buf, size_t buf_size) {
    int dma_channel = dma_claim_unused_channel(true);
    assert(dma_channel);
    dma_channel_config rx_dma_config = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&rx_dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&rx_dma_config, false);
    channel_config_set_write_increment(&rx_dma_config, true);
    channel_config_set_dreq(&rx_dma_config, pio_dreq); // Typically this will be: DREQ_PIO0_RX1
    dma_channel_configure(
        dma_channel,
        &rx_dma_config,
        raw_rx_buf,
        pio_rx_fifo,
        buf_size,
        false       // Delay DMA start
    );
    dma_channel_set_irq0_enable(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, tcpc_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(dma_channel);
    return dma_channel;
}
static void tcpc_poll_dma(tcpcPhyChannel *phy_ch) {
    uint dma_transfer_idx = phy_ch->raw_buf_rx_size - dma_channel_hw_addr(phy_ch->dma_rx)->transfer_count;
    uint *process_count = &phy_ch->process_idx_rx;
    if(dma_transfer_idx == *process_count) {
        // No new data received
        return;
    }
    // New data was received
    uint32_t *raw_data = &(phy_ch->raw_buf_rx[*process_count]);
    printf("%s", *raw_data);
}

void tcpc_task(void *arg) {
    (void)arg;
    tcpcPhyChannel tcpc_phy_chan = {
        pio0,
        0,
        PIO0_IRQ_0,
        6,
    };
    

}
