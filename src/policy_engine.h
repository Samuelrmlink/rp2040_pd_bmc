#ifndef _POLICY_ENGINE_H
#define _POLICY_ENGINE_H

typedef enum {
    KeyString,
    KeyU32,
    KeyBitval,
} keyDataType;
struct keyBitVal {
    const uint8_t regNum;
    const uint8_t _unused;
    const uint8_t lsbOffset;    // Range: [0, 31]
    const uint8_t msbOffset;    // Range: [0, 31]
    const uint32_t minValue;
    const uint32_t maxValue;
};
struct configKey {
    const char* name;
    const char* desc;
    const uint8_t keyDataType;
    const void* keyPtr;
};




typedef struct usbpdPolicy usbpdPolicy;
struct usbpdPolicy {

};

#endif