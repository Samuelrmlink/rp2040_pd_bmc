#ifndef _BMC_RX_H
#define _BMC_RX_H
#include "hardware/pio.h"

typedef struct bmcRx bmcRx;
typedef struct bmcTx bmcTx;

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

    uint16_t overflowCount;     // Counts the number of times pio rx buffer has been completely full
    uint8_t lastOverflow;       // Index of the last pio rx buffer overflow
};

struct bmcTx {
    pd_frame *pdf;
    uint8_t byteOffset;
    bool upperSymbol;
    uint8_t scrapBits;
    uint8_t numScrapBits;
    uint8_t msgIdOut;       // Outgoing msgID value [actually only 3 bits - please use tx_msg_inc(<pointer to this variable>) to increment]
    uint8_t num_u32;        // Number of u32 objects to send to the PIO
    uint32_t *out;          // Pointer to the first aformentioned u32 object
    uint16_t num_zeros;     // Number of zeros (to skip when transmitting via PIO)
};

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

struct bmcChannels {
    uint8_t maxChannels;	// Number of channels in pointer array
    bmcChannel *chan;		// Pointer to the first channel element
};
typedef struct bmcChannels bmcChannels;

#endif
