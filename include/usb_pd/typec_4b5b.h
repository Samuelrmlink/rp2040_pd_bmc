#ifndef _TYPEC_4B5B_H
#define _TYPEC_4B5B_H

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


#endif
