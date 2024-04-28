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

/*
struct bmcEncode {
    pd_frame *pdf;
    uint32_t crc;
    uint8_t num_u32;
    uint32_t *out;

    uint8_t procStage;
    uint32_t procSubStage;
    uint32_t remainBits;
    uint8_t numRemainBits;

};
*/

#endif
