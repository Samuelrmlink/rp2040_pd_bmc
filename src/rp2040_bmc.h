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
