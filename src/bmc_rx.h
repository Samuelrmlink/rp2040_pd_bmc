#ifndef _BMC_RX_H
#define _BMC_RX_H

typedef struct bmcRx bmcRx;

struct bmcRx {
    pd_frame *pdfPtr;           // Pointer to the first pd_frame object allocated
    uint8_t rolloverObj;        // Total number of pd_frame objects allocated
    uint8_t objOffset;          // Offset # of pd_frame objects (starts at 0 - should rollover and never actually land on the rollover_obj value)
    uint8_t byteOffset;         // Offset # of bytes
    bool upperSymbol;           // Each PD symbol is 4 bits - true loads the upper bits [7..4] instead of the lower bits [3..0]
    bool inputRollover;

    uint8_t scrapBits;          // Leftover bits from the last frame are stored here
    uint8_t afterScrapOffset;   // Number of leftover bits [<< shift direction]
    uint8_t inputOffset;        // Input offset {>> shift direction]
    uint8_t rx_crc32;           // CRC32 value - for validation purposes
    bool eval_crc32;            // Validate CRC32 if true
};

#endif
