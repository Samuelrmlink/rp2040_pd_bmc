#include "main_i.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"

// Utilized by the 'tcpc_dma_handler' function
uint tcpc_dma_channel;
uint32_t *tcpc_dma_buf_rx;

static int tcpc_pio_rx_init(PIO pio, uint sm_rx, uint pin_rx) {
    float clock_div = (float)clock_get_hz(clk_sys) / 5000000;
    uint sm_rx_offset = pio_add_program(pio, &differential_manchester_rx_program);
    differential_manchester_rx_program_init(pio, sm_rx, sm_rx_offset, pin_rx, clock_div);
    pio_sm_set_enabled(pio, sm_rx, true);
}
static int tcpc_pio_tx_init(PIO pio, uint sm_tx, uint pin_tx_high) {
    float clock_div = (float)clock_get_hz(clk_sys) / 5000000;
    uint sm_tx_offset = pio_add_program(pio, &differential_manchester_tx_program);
    differential_manchester_tx_program_init(pio, sm_tx, sm_tx_offset, pin_tx_high, clock_div);
    pio_sm_set_enabled(pio, sm_tx, true);
}

static void tcpc_dma_handler() {
    extern uint tcpc_dma_channel;
    extern uint32_t *tcpc_dma_buf_rx;
    // Clear DMA interrupt
    dma_hw->ints0 = 1u << tcpc_dma_channel;
    // Reset write address
    dma_channel_set_write_addr(tcpc_dma_channel, tcpc_dma_buf_rx, true);
}
//  
//  tcpc_rx_dma_init - Initializes USB-PD RX DMA channel
//
//  pio_rx_fifo     : Pointer to PIO RX FIFO
//  pio_dreq        : PIO DREQ handle
//  raw_rx_buf      : Received Data Buffer
//  buf_size        : Received Data Buffer Size
static int tcpc_rx_dma_init(const volatile io_ro_32 *pio_rx_fifo, uint pio_dreq, void *raw_rx_buf, size_t buf_size) {
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
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, tcpc_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_start(dma_channel);
    return dma_channel;
}
static void tcpc_poll_dma(tcpcPhyChannel *phy_ch) {
    static uint32_t last_frame_timestamp;
    static pd_frame current_frame;
    pd_frame goodcrc_resp_frame;
    uint dma_transfer_idx = phy_ch->raw_buf_rx_size - dma_channel_hw_addr(phy_ch->dma_rx)->transfer_count;
    uint *process_count = &phy_ch->process_idx_rx;
    // Reset *process_count if we are past max
    if(*process_count >= phy_ch->raw_buf_rx_size) {
        *process_count = 0;
    }
    if(dma_transfer_idx == *process_count) {
        // No new data received
        if(last_frame_timestamp + 200 < time_us_32() && current_frame.timestamp_us) {
            // There should be a new pd_frame or EOP symbol
            // We need to get the PIO SM to push its ISR to the FIFO
            pio_sm_exec(phy_ch->pio, phy_ch->sm_rx, pio_encode_in(pio_y, 1));
        }
        return;
    } else {
        // New data was received
        debug_pin_toggle(16);
        last_frame_timestamp = time_us_32();
        uint32_t *raw_data = &(phy_ch->raw_buf_rx[*process_count]);
        if(typec_4b5b_decode(&current_frame, *raw_data)) {
            // Valid frame was received
            if(typec_pdframe_valid(&current_frame) && typec_pdframe_orderedset_get_idx(current_frame.ordered_set) == pdfTypeSop && !typec_pdframe_compare(&current_frame, &goodcrc_resp_frame, sizeof(pd_frame))) {
                // Clear response frame
                memset(&goodcrc_resp_frame, 0, sizeof(pd_frame));
                // Write GoodCRC response
                typec_pdframe_generate_goodcrc(&current_frame, &goodcrc_resp_frame);
                // Generate raw PIO data stream
                tcpcBmcPhyTxData *raw_frame = tcpc_bmc_phy_tx_prepare(&goodcrc_resp_frame);
                // Send raw PIO data stream
                tcpc_bmc_phy_tx_send(phy_ch, raw_frame);
                free(raw_frame);
            }
//            if(typec_pdframe_valid(&current_frame)) { printf("V %X %X\n", current_frame.hdr, current_frame.ordered_set); } else { printf("Iv %X %X\n", current_frame.hdr, current_frame.ordered_set); }
            memset(&current_frame, 0, sizeof(pd_frame));
        }
        (*process_count)++;
    }
}

tcpcPhyChannel tcpc_phy_chan = {
    pio0,           // pio
    0,              // sm_tx
    1,              // sm_rx
    PIO0_IRQ_0,     // irq
    6,              // pin_rx
    9,              // pin_tx_high
    10,             // pin_tx_low
    0,              // dma_rx
    0,              // *raw_buf_rx
    40,             // raw_buf_rx_size
    0               // process_idx_rx
};

void tcpc_task(void *arg) {
    (void)arg;
    extern tcpcPhyChannel tcpc_phy_chan;
    tcpc_phy_chan.raw_buf_rx = malloc(sizeof(uint32_t) * tcpc_phy_chan.raw_buf_rx_size);
    memset(tcpc_phy_chan.raw_buf_rx, 0, sizeof(uint32_t) * tcpc_phy_chan.raw_buf_rx_size);
    tcpc_phy_chan.dma_rx = tcpc_rx_dma_init(
        &(pio0_hw->rxf[tcpc_phy_chan.sm_rx]),
        DREQ_PIO0_RX1, // TODO : Dynamically assign?
        tcpc_phy_chan.raw_buf_rx,
        tcpc_phy_chan.raw_buf_rx_size
    );
    tcpc_dma_channel = tcpc_phy_chan.dma_rx;
    tcpc_dma_buf_rx = tcpc_phy_chan.raw_buf_rx;
    tcpc_pio_rx_init(tcpc_phy_chan.pio, tcpc_phy_chan.sm_rx, tcpc_phy_chan.pin_rx);
    tcpc_pio_tx_init(tcpc_phy_chan.pio, tcpc_phy_chan.sm_tx, tcpc_phy_chan.pin_tx_high);

    //typec_operation_test();

    while(true) {
        tcpc_poll_dma(&tcpc_phy_chan);
    }
}
