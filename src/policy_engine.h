#ifndef _POLICY_ENGINE_H
#define _POLICY_ENGINE_H

typedef struct {
    const uint8_t regNum;
    const uint8_t _unused;
    const uint8_t lsbOffset;    // Range: [0, 31]
    const uint8_t msbOffset;    // Range: [0, 31]
    const uint32_t minValue;
    const uint32_t maxValue;
} keyBitValue;
typedef union {
    char String[20];
    uint32_t U32;
    keyBitValue Bv;
} keyData;
typedef enum {
    KeyString,
    KeyU32,
    KeyBitval,
} keyDataTypes;
typedef struct {
    const char* name;
    const char* desc;
    const uint8_t keyDataType; // Set according to keyDataTypes enum above
    const void* keyPtr;
    const bool nvmBackup;
} configKey;

static keyData key_mv_min = {
    .Bv = {
        .regNum = 1,
        .lsbOffset = 2,
        .msbOffset = 3,
        .minValue = 4,
        .maxValue = 5,
    },
};
static configKey database[] = {
    {
        .name = "mv_min",
        .desc = "[Power Sink]: Minimum Voltage",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_min,
        .nvmBackup = true,
    },
};
//static configKey* config_db = database;


typedef struct usbpdPolicy usbpdPolicy;
struct usbpdPolicy {

};

#endif