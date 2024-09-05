#ifndef _BMC_RX_H
#define _BMC_RX_H

typedef struct bmcRx bmcRx;

struct bmcRx {
    pd_frame *pdfPtr;       // Pointer to the first pd_frame object allocated
    uint8_t rolloverObj;    // Total number of pd_frame objects allocated
    uint8_t objOffset;     // Offset # of pd_frame objects (starts at 0 - should rollover and never actually land on the rollover_obj value)
    uint8_t byteOffset;    // Offset # of bytes
    bool evenSymbol;       // Each PD symbol is 4 bits - true loads the upper bits [7..4] instead of the lower bits [3..0]

    uint32_t procBuf;
    uint8_t inputBitReadPosition;
    uint8_t remainOffset;
};

#endif
