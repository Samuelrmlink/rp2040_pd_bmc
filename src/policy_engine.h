#ifndef _POLICY_ENGINE_H
#define _POLICY_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_NUMBER_OF_REGISTERS 3
extern uint32_t config_reg[CONFIG_NUMBER_OF_REGISTERS];

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
    const keyData* keyPtr;
    const bool nvmBackup;
} configKey;

static keyData key_mv_min = { .Bv = { .regNum = 1, .lsbOffset = 0, .msbOffset = 9, .minValue = 66, .maxValue = 1000 } };    // Range: [3.3v, 50v]
static keyData key_mv_max = { .Bv = { .regNum = 1, .lsbOffset = 10, .msbOffset = 19, .minValue = 66, .maxValue = 1000 } };  // Range: [3.3v, 50v]
static keyData key_ma_min = { .Bv = { .regNum = 2, .lsbOffset = 0, .msbOffset = 9, .minValue = 0, .maxValue = 500 } };      // Range: [0a, 5a]
static keyData key_ma_max = { .Bv = { .regNum = 2, .lsbOffset = 10, .msbOffset = 19, .minValue = 0, .maxValue = 500 } };    // Range: [0a, 5a]
static configKey database[] = {
    {
        .name = "sink.volt_min_raw",
        .desc = "[Power Sink]: Minimum Voltage in 50mV units",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_min,
        .nvmBackup = true,
    },
    {
        .name = "sink.volt_max_raw",
        .desc = "[Power Sink]: Maximum Voltage in 50mV units",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_max,
        .nvmBackup = true,
    },
    {
        .name = "sink.cur_min_raw",
        .desc = "[Power Sink]: Minimum Current in 10mA units",
        .keyDataType = KeyBitval,
        .keyPtr = &key_ma_min,
        .nvmBackup = true,
    },
    {
        .name = "sink.cur_max_raw",
        .desc = "[Power Sink]: Maximum Current in 10mA units",
        .keyDataType = KeyBitval,
        .keyPtr = &key_ma_max,
        .nvmBackup = true,
    },
};
const size_t database_items_count = sizeof(database) / sizeof(configKey);

typedef struct usbpdPolicy usbpdPolicy;
struct usbpdPolicy {

};

#ifdef __cplusplus
}
#endif

#endif