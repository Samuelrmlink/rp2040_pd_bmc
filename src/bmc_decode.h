#ifndef _BMC_DECODE_H
#define _BMC_DECODE_H

typedef struct bmcDecode bmcDecode;

typedef struct {
    uint32_t val;
    uint32_t time;
} rx_data;

struct bmcDecode {
    uint8_t procStage;
    uint32_t procSubStage;
    rx_data pioData;
    uint32_t procBuf;
    uint8_t pOffset;
    uint32_t crcTmp;
    pd_frame *msg;
};

int bmcProcessSymbols(bmcDecode* bmc_d, QueueHandle_t q_validPdf);

#endif
