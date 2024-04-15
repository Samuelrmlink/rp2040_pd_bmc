#ifndef _BMC_4B5B_H
#define _BMC_4B5B_H

#define NUM_BITS_ORDERED_SET 20

typedef enum {
    // Data Symbols
    symHex0	= 0b11110,
    symHex1	= 0b01001,
    symHex2	= 0b10100,
    symHex3	= 0b10101,
    symHex4	= 0b01010,
    symHex5	= 0b01011,
    symHex6	= 0b01110,
    symHex7	= 0b01111,
    symHex8	= 0b10010,
    symHex9	= 0b10011,
    symHexA	= 0b10110,
    symHexB	= 0b10111,
    symHexC	= 0b11010,
    symHexD	= 0b11011,
    symHexE	= 0b11100,
    symHexF	= 0b11101,

    // K-Code Symbols
    symKcodeS1	= 0b11000,
    symKcodeS2	= 0b10001,
    symKcodeR1	= 0b00111,
    symKcodeR2	= 0b11001,
    symKcodeEop	= 0b01101,
    symKcodeS3	= 0b00110
} BMC5bSymbols;

typedef enum {
    ordsetHardReset =   symKcodeR1 |
                        symKcodeR1 << 5 |
                        symKcodeR1 << 10 |
                        symKcodeR2 << 15,
    ordsetCableReset =  symKcodeR1 |
                        symKcodeS1 << 5 |
                        symKcodeR1 << 10 |
                        symKcodeS3 << 15,
    ordsetSop =         symKcodeS1 |
                        symKcodeS1 << 5 |
                        symKcodeS1 << 10 |
                        symKcodeS2 << 15,
    ordsetSopP =        symKcodeS1 |
                        symKcodeS1 << 5 |
                        symKcodeS3 << 10 |
                        symKcodeS3 << 15,
    ordsetSopDp =       symKcodeS1 |
                        symKcodeS3 << 5 |
                        symKcodeS1 << 10 |
                        symKcodeS3 << 15,
    ordsetSopPDbg =     symKcodeS1 |
                        symKcodeR2 << 5 |
                        symKcodeR2 << 10 |
                        symKcodeS3 << 15,
    ordsetSopDpDbg =    symKcodeS1 |
                        symKcodeR2 << 5 |
                        symKcodeS3 << 10 |
                        symKcodeS2 << 15,
} BMC5bOrderedSets;

static const uint8_t const bmc4bTo5b[] = { 
    symHex0, symHex1, symHex2, symHex3, 
    symHex4, symHex5, symHex6, symHex7, 
    symHex8, symHex9, symHexA, symHexB, 
    symHexC, symHexD, symHexE, symHexF 
};

static const uint8_t const bmc5bTo4b[] = {
    [(uint8_t)symHex0]	= 0x0,
    [(uint8_t)symHex1]	= 0x1,
    [(uint8_t)symHex2]	= 0x2,
    [(uint8_t)symHex3]	= 0x3,
    [(uint8_t)symHex4]	= 0x4,
    [(uint8_t)symHex5]	= 0x5,
    [(uint8_t)symHex6]	= 0x6,
    [(uint8_t)symHex7]	= 0x7,
    [(uint8_t)symHex8]	= 0x8,
    [(uint8_t)symHex9]	= 0x9,
    [(uint8_t)symHexA]	= 0xA,
    [(uint8_t)symHexB]	= 0xB,
    [(uint8_t)symHexC]	= 0xC,
    [(uint8_t)symHexD]	= 0xD,
    [(uint8_t)symHexE]	= 0xE,
    [(uint8_t)symHexF]	= 0xF
};

#endif
