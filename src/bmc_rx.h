#ifndef _BMC_RX_H
#define _BMC_RX_H

typedef struct bmcRx bmcRx;

struct bmcRx {
    pd_frame *pdf_ptr;      // Pointer to the first pd_frame object allocated
    uint8_t rollover_obj;   // Total number of pd_frame objects allocated
    uint8_t obj_offset;     // Offset # of pd_frame objects (starts at 0 - should rollover and never actually land on the rollover_obj value)
    uint8_t byte_offset;    // Offset # of bytes
    bool even_symbol;       // Each PD symbol is 4 bits - true loads the upper bits [7..4] instead of the lower bits [3..0]
};

#endif
