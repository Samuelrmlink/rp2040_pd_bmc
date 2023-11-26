#ifndef _BMC_DECODE_H
#define _BMC_DECODE_H

typedef struct bmcDecode bmcDecode;

struct bmcDecode {
    uint8_t procStage;
    uint32_t procSubStage;
    uint32_t inBuf;
    uint32_t procBuf;
    uint8_t pOffset;
    uint32_t rxTime;
    uint32_t crcTmp;
};

int bmcProcessSymbols(bmcDecode* bmc_d, pd_frame* msg);

#endif
