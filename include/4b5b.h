#ifndef _BMC_4B5B_H
#define _BMC_4B5B_H

#define NUM_BITS_ORDERED_SET 20
#define NUM_BITS_SYMBOL 5

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
    sym4bKcodeS1    = 0x10,
    sym4bKcodeS2    = 0x11,
    sym4bKcodeR1    = 0x12,
    sym4bKcodeR2    = 0x13,
    sym4bKcodeEop   = 0x14,
    sym4bKcodeS3    = 0x15
} BMC4bSymbols;

typedef enum {
    ordsetHardReset =   sym4bKcodeR1 |
                        sym4bKcodeR1 << 8 |
                        sym4bKcodeR1 << 16 |
                        sym4bKcodeR2 << 24,
    ordsetCableReset =  sym4bKcodeR1 |
                        sym4bKcodeS1 << 8 |
                        sym4bKcodeR1 << 16 |
                        sym4bKcodeS3 << 24,
    ordsetSop =         sym4bKcodeS1 |
                        sym4bKcodeS1 << 8 |
                        sym4bKcodeS1 << 16 |
                        sym4bKcodeS2 << 24,
    ordsetSopP =        sym4bKcodeS1 |
                        sym4bKcodeS1 << 8 |
                        sym4bKcodeS3 << 16 |
                        sym4bKcodeS3 << 24,
    ordsetSopDp =       sym4bKcodeS1 |
                        sym4bKcodeS3 << 8 |
                        sym4bKcodeS1 << 16 |
                        sym4bKcodeS3 << 24,
    ordsetSopPDbg =     sym4bKcodeS1 |
                        sym4bKcodeR2 << 8 |
                        sym4bKcodeR2 << 16 |
                        sym4bKcodeS3 << 24,
    ordsetSopDpDbg =    sym4bKcodeS1 |
                        sym4bKcodeR2 << 8 |
                        sym4bKcodeS3 << 16 |
                        sym4bKcodeS2 << 24
} BMC4bOrderedSets;

// Returns the 20 bit Ordered Set
static const uint32_t const bmcFrameType[] = {
    0,			// PdfTypeInvalid
    ordsetHardReset,	// PdfTypeHardReset
    ordsetCableReset,	// PdfTypeCableReset
    ordsetSop,		// PdfTypeSop
    ordsetSopP,		// PdfTypeSopP
    ordsetSopDp,	// PdfTypeSopDp
    ordsetSopPDbg,	// PdfTypeSopPDbg
    ordsetSopDpDbg	// PdfTypeSopDpDbg
};

typedef enum {
    pdfTypeInvalid,	    // 0
    pdfTypeHardReset,	// 1
    pdfTypeCableReset,	// 2
    pdfTypeSop,		    // 3
    pdfTypeSopP,	    // 4
    pdfTypeSopDp,	    // 5
    pdfTypeSopPDbg,	    // 6
    pdfTypeSopDpDbg	    // 7
} pdfTypes;

static const uint8_t const bmc4bTo5b[] = { 
    symHex0, symHex1, symHex2, symHex3, 
    symHex4, symHex5, symHex6, symHex7, 
    symHex8, symHex9, symHexA, symHexB, 
    symHexC, symHexD, symHexE, symHexF, 
    symKcodeS1, symKcodeS2, symKcodeR1,
    symKcodeR2, symKcodeEop, symKcodeS3
};

static const uint8_t const bmc5bTo4b[] = {
    [(uint8_t)symHex0]	    = 0x0,
    [(uint8_t)symHex1]	    = 0x1,
    [(uint8_t)symHex2]	    = 0x2,
    [(uint8_t)symHex3]	    = 0x3,
    [(uint8_t)symHex4]	    = 0x4,
    [(uint8_t)symHex5]	    = 0x5,
    [(uint8_t)symHex6]	    = 0x6,
    [(uint8_t)symHex7]	    = 0x7,
    [(uint8_t)symHex8]	    = 0x8,
    [(uint8_t)symHex9]	    = 0x9,
    [(uint8_t)symHexA]	    = 0xA,
    [(uint8_t)symHexB]	    = 0xB,
    [(uint8_t)symHexC]	    = 0xC,
    [(uint8_t)symHexD]	    = 0xD,
    [(uint8_t)symHexE]	    = 0xE,
    [(uint8_t)symHexF]	    = 0xF,
    [(uint8_t)symKcodeS1]	= 0x10,
    [(uint8_t)symKcodeS2]	= 0x11,
    [(uint8_t)symKcodeR1]	= 0x12,
    [(uint8_t)symKcodeR2]	= 0x13,
    [(uint8_t)symKcodeEop]	= 0x14,
    [(uint8_t)symKcodeS3]	= 0x15
};

#endif
