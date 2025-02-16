#pragma once

#ifndef _DEVICE_REGISTERS_H
#define _DEVICE_REGISTERS_H


#define CONFIG_NUMBER_OF_REGISTERS 4
extern uint32_t config_reg[CONFIG_NUMBER_OF_REGISTERS];

#define CONFIG_NUMBER_OF_STRINGS 1
extern char* string_ptr[CONFIG_NUMBER_OF_STRINGS];

typedef struct {
    const uint8_t regNum;
    const uint8_t valMultp;     // Value multiplier [0, 255]
    const uint8_t lsbOffset;    // Range: [0, 31]
    const uint8_t msbOffset;    // Range: [0, 31]
    const uint32_t minValue;    // Minimum [Register] value (no multiplier applied)
    const uint32_t maxValue;    // Maximum [Register] value (no multiplier applied)]
    const uint32_t defaultValue;// Default register value
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

static keyData key_mv_min = { .Bv = { .regNum = 1, .valMultp = 50, .lsbOffset = 0, .msbOffset = 9, .minValue = 66, .maxValue = 1000, .defaultValue = 100 } };       // Range: [3.3v, 50v]
static keyData key_mv_max = { .Bv = { .regNum = 1, .valMultp = 50, .lsbOffset = 10, .msbOffset = 19, .minValue = 66, .maxValue = 1000, .defaultValue = 100 } };     // Range: [3.3v, 50v]
static keyData key_ma_min = { .Bv = { .regNum = 2, .valMultp = 10, .lsbOffset = 0, .msbOffset = 9, .minValue = 0, .maxValue = 500, .defaultValue = 10 } };          // Range: [0a, 5a]
static keyData key_ma_max = { .Bv = { .regNum = 2, .valMultp = 10, .lsbOffset = 10, .msbOffset = 19, .minValue = 0, .maxValue = 500, .defaultValue = 300 } };       // Range: [0a, 5a]
static keyData key_sop_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 1 } };        // SOP Accept               (Sends GoodCRC & Forwards pd_frame to Policy Engine if set)
static keyData key_sopp_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 0 } };       // SOP' Accept               |
static keyData key_sopdp_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 0 } };      // SOP" Accept               |
static keyData key_soppd_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 0 } };      // SOP' Debug Accept         |
static keyData key_sopdpd_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 0 } };     // SOP" Debug Accept         X
static keyData key_sft_rst_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 1 } };    // SOP Accept               (Notifies Policy Engine if set)
static keyData key_hrd_rst_accept = { .Bv = { .regNum = 1, .valMultp = 1, .lsbOffset = 20, .msbOffset = 20, .minValue = 0, .maxValue = 1, .defaultValue = 1 } };    // SOP Accept                X

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
        .keyDataType = KeyBitval,
        .keyPtr = &key_test,
        .nvmBackup = true,
        .useMultp = false,
    },
    {
        .name = "accept.sop",
        .desc = "[Accept]: SOP Communication { Power Source/Sink <--> picoPD_interrogator }",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },
    {
        .name = "accept.sopp",
        .desc = "[Accept]: SOP' Communication { Cable E-Marker chip <--> picoPD_interrogator }",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },{
        .name = "accept.sopdp",
        .desc = "[Accept]: SOP\" Communication { Cable E-Marker chip <--> picoPD_interrogator }",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },{
        .name = "accept.soppd",
        .desc = "[Accept]: SOP' Debug Communication { Cable/Accessory Debug Interface}",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },{
        .name = "accept.sopdpd",
        .desc = "[Accept]: SOP\" Debug Communication { Cable/Accessory Debug Interface }",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },{
        .name = "accept.softreset",
        .desc = "[Accept]: Soft Reset frame",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },{
        .name = "accept.hardreset",
        .desc = "[Accept]: Hard Reset frame",
        .keyDataType = KeyBitval,
        .keyPtr = &key_sop_accept,
        .nvmBackup = false,
        .useMultp = false,
    },
};
/*
typedef struct usbpdPolicy usbpdPolicy;
struct usbpdPolicy {

};
*/

uint32_t mcu_reg_get_uint(keyData* key, bool use_multiplier);



#endif
