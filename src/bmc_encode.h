#ifndef _BMC_ENCODE_H
#define _BMC_ENCODE_H

#define TX_VALUE_PREAMBLE_ADVANCE 2 // 0b10
#define NUM_BITS_PREAMBLE_ADVANCE 2

struct txFrame {
    pd_frame *pdf;
    uint32_t crc;
    uint8_t msgIdOut; // Outgoing msgID value (actually only 3 bits - please use tx_msg_inc() to increment)

    uint8_t num_u32;
    uint32_t *out;
    uint16_t num_zeros; // Number of zeros (to skip when transmitting via PIO)
};
typedef struct txFrame txFrame;

void tx_msg_inc(uint8_t *msgId);
void pdf_generate_goodcrc(pd_frame *input_frame, txFrame *tx);
void pdf_request_from_srccap(pd_frame *input_frame, txFrame *tx, uint8_t req_pdo);
void pdf_generate_source_capabilities_basic(pd_frame *input_frame, txFrame *tx);
void static tx_raw_buf_write(uint32_t input_bits, uint8_t num_input_bits, uint32_t *buf, uint16_t *buf_position);
void pdf_to_uint32(txFrame *txf);
bool bmc_rx_active(bmcChannel *chan);
void pdf_transmit(txFrame *txf, bmcChannel *ch);

#endif
