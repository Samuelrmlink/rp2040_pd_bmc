#ifndef _RP2040_BMC_H
#define _RP2040_BMC_H

#include "main_i.h"

struct bmcChannel {
    // Hardware PIO & state machine handles
    PIO pio;
    uint sm_tx;
    uint sm_rx;
    uint irq;	// Example: PIO0_IRQ_0, etc...

    // Hardware pins
    uint rx;
    uint tx_high;
    uint tx_low;
    uint adc;

    // Currently unused (TO BE ADDED) - TODO:
    // connector_orientation
    // vconn_state
};
typedef struct bmcChannel bmcChannel;
extern bmcChannel *bmc_ch0;

/*
 *	bmc.c
 *	bmc_encode.c
 *	bmc_decode.c
 *
 */
void bmc_rx_check();
void bmc_rx_cb();
bmcChannel* bmc_channel0_init();
void individual_pin_toggle(uint8_t pin_num);

void pd_frame_queue_and_reset(bmcDecode* bmc_d, QueueHandle_t q_validPdf);
int bmcProcessSymbols(bmcDecode* bmc_d, QueueHandle_t q_validPdf);

void tx_msg_inc(uint8_t *msgId);
void pdf_generate_goodcrc(pd_frame *input_frame, txFrame *tx);
void pdf_request_from_srccap(pd_frame *input_frame, txFrame *tx, uint8_t req_pdo);
void pdf_generate_source_capabilities_basic(pd_frame *input_frame, txFrame *tx);
void static tx_raw_buf_write(uint32_t input_bits, uint8_t num_input_bits, uint32_t *buf, uint16_t *buf_position);
void pdf_to_uint32(txFrame *txF);
bool bmc_rx_active(bmcChannel *chan);
void pdf_transmit(txFrame *txf, bmcChannel *ch);


/* 
 *	usb_pd.c
 *
 */
void bmc_decode_clear(bmcDecode* bmc_d);
void pd_frame_clear(pd_frame* pdf);
bool is_crc_good(pd_frame *pdf);
bool is_sop_frame(pd_frame *pdf);
PDMessageType pdf_get_sop_msg_type(pd_frame *msg);
bool is_src_cap(pd_frame *pdf);
uint8_t optimal_pdo(pd_frame *pdf, uint16_t req_mvolts);
//void pdf_generate_request(pd_frame *pdf, txFrame *txf, uint8_t req_index);


void bmc_rx_check();
void bmc_rx_cb();
bmcChannel* bmc_channel0_init();

void individual_pin_toggle(uint8_t pin_num); // TODO: possibly move to GPIO header file
void thread_rx_process(void* unused_arg);

#endif
