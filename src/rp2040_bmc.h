#ifndef _RP2040_BMC_H
#define _RP2040_BMC_H

#include "main_i.h"

extern bmcChannel *bmc_ch0;

/*
 *	bmc.c
 *	bmc_decode.c
 *	bmc_encode.c
 *
 */
bmcRx* bmc_rx_setup();
uint32_t bmc_get_timestamp(bmcRx *rx);
bool bmc_validate_pdf(pd_frame *pdf);
uint8_t bmc_get_ordset_index(uint32_t input);
uint8_t bmc_extended_unchunked_bytes(pd_frame *pdf);
void bmc_locate_preamble(bmcRx *rx, uint32_t *in);
void bmc_locate_sof(bmcRx *rx, uint32_t *in);
uint8_t bmc_pull_5b(bmcRx *rx, uint32_t *pio_raw);
bool bmc_load_symbols(bmcRx *rx, uint32_t *pio_raw);
void bmc_process_symbols(bmcRx *rx, uint32_t *pio_raw);
void bmc_rx_check();
void bmc_rx_cb();
bmcChannel* bmc_channel0_init();
void individual_pin_toggle(uint8_t pin_num);
void static tx_raw_buf_write(uint32_t input_bits, uint8_t num_input_bits, uint32_t *buf, uint16_t *buf_position);
void pdf_to_uint32(bmcTx *txf);
bool bmc_rx_active(bmcChannel *chan);
void pdf_transmit(bmcTx *txf, bmcChannel *ch);


void tx_msg_inc(uint8_t *msgId);
void pdf_request_from_srccap_fixed(pd_frame *input_frame, bmcTx *tx, uint8_t req_pdo, pdo_accept_criteria req);
void pdf_request_from_srccap_augmented(pd_frame *input_frame, bmcTx *tx, uint8_t req_pdo, pdo_accept_criteria req);
//void pdf_generate_source_capabilities_basic(pd_frame *input_frame, txFrame *tx);


/* 
 *	usb_pd.c
 *
 */
void pd_frame_clear(pd_frame* pdf);
void pdf_generate_goodcrc(pd_frame *input_frame, pd_frame *output_frame);
PDMessageType pdf_get_sop_msg_type(pd_frame *msg);
bool is_src_cap(pd_frame *pdf);
uint8_t optimal_pdo(pd_frame *pdf, pdo_accept_criteria power_req);
//void pdf_generate_request(pd_frame *pdf, txFrame *txf, uint8_t req_index);
void thread_rx_process(void* unused_arg);

#endif
