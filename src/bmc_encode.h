#ifndef _BMC_ENCODE_H
#define _BMC_ENCODE_H

struct txFrame {
    pd_frame *pdf;
    uint32_t crc;

    uint8_t num_u32;
    uint32_t *out;
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
