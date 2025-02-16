#ifndef _DEVICE_REGISTERS_H
#define _DEVICE_REGISTERS_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONFIG_NUMBER_OF_REGISTERS 3
extern uint32_t config_reg[CONFIG_NUMBER_OF_REGISTERS];

#define CONFIG_NUMBER_OF_STRINGS 1
extern char* string_ptr[CONFIG_NUMBER_OF_STRINGS];

typedef struct {
    const uint8_t regNum;
    const uint8_t valMultp;     // Value multiplier [0, 255]
    const uint8_t lsbOffset;    // Range: [0, 31]
    const uint8_t msbOffset;    // Range: [0, 31]
    const uint32_t minValue;    // Minimum [Register] value (no multiplier applied)
    const uint32_t maxValue;    // Maximum [Register] value (no multiplier applied)
} keyBitValue;
typedef struct {
    char** const ptr_str_ptr;   // Pointer to string pointer (NULL when undefined)
    const char* default_str;    // Default string pointer
} keyString;
typedef union {
    keyString Ks;
    keyBitValue Bv;
} keyData;
typedef enum {
    KeyString,
    KeyBitval,
} keyDataTypes;
typedef struct {
    const char* name;
    const char* desc;
    const uint8_t keyDataType; // Set according to keyDataTypes enum above
    const keyData* keyPtr;
    const bool nvmBackup;
    const bool useMultp;       // Use multiplier (otherwise raw register value is returned)
} configKey;

static keyData key_mv_min = { .Bv = { .regNum = 1, .valMultp = 50, .lsbOffset = 0, .msbOffset = 9, .minValue = 66, .maxValue = 1000 } };    // Range: [3.3v, 50v]
static keyData key_mv_max = { .Bv = { .regNum = 1, .valMultp = 50, .lsbOffset = 10, .msbOffset = 19, .minValue = 66, .maxValue = 1000 } };  // Range: [3.3v, 50v]
static keyData key_ma_min = { .Bv = { .regNum = 2, .valMultp = 10, .lsbOffset = 0, .msbOffset = 9, .minValue = 0, .maxValue = 500 } };      // Range: [0a, 5a]
static keyData key_ma_max = { .Bv = { .regNum = 2, .valMultp = 10, .lsbOffset = 10, .msbOffset = 19, .minValue = 0, .maxValue = 500 } };    // Range: [0a, 5a]
static keyData key_test = { .Ks = { .ptr_str_ptr = &(string_ptr[0]), .default_str = "TEST default :D" } };
static configKey database[] = {
    {
        .name = "sink.volt_min_raw",
        .desc = "[Power Sink]: Minimum Voltage in 50mV units (Raw)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_min,
        .nvmBackup = true,
        .useMultp = false,
    },
    {
        .name = "sink.volt_max_raw",
        .desc = "[Power Sink]: Maximum Voltage in 50mV units (Raw)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_max,
        .nvmBackup = true,
        .useMultp = false,
    },
    {
        .name = "sink.cur_min_raw",
        .desc = "[Power Sink]: Minimum Current in 10mA units (Raw)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_ma_min,
        .nvmBackup = true,
        .useMultp = false,
    },
    {
        .name = "sink.cur_max_raw",
        .desc = "[Power Sink]: Maximum Current in 10mA units (Raw)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_ma_max,
        .nvmBackup = true,
        .useMultp = false,
    },{
        .name = "sink.volt_min",
        .desc = "[Power Sink]: Minimum Voltage (mV)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_min,
        .nvmBackup = true,
        .useMultp = true,
    },
    {
        .name = "sink.volt_max",
        .desc = "[Power Sink]: Maximum Voltage (mV)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_mv_max,
        .nvmBackup = true,
        .useMultp = true,
    },
    {
        .name = "sink.cur_min",
        .desc = "[Power Sink]: Minimum Current (mA)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_ma_min,
        .nvmBackup = true,
        .useMultp = true,
    },
    {
        .name = "sink.cur_max",
        .desc = "[Power Sink]: Maximum Current (mA)",
        .keyDataType = KeyBitval,
        .keyPtr = &key_ma_max,
        .nvmBackup = true,
        .useMultp = true,
    },
    {
        .name = "test.string",
        .desc = "[Test]: Just a test string",
        .keyDataType = KeyString,
        .keyPtr = &key_test,
        .nvmBackup = true,
        .useMultp = false,
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