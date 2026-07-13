#ifndef _SAMHEDRICK_PICO_PDINTERROGATOR_REV_D_RP2350_H
#define _SAMHEDRICK_PICO_PDINTERROGATOR_REV_D_RP2350_H

// For board detection
#define SAMHEDRICK_PICO_PDINTERROGATOR_REV_D_RP2350

pico_board_cmake_set(PICO_PLATFORM, rp2350)

// VCONN Enables (power E-Marker cable assemblies)
#define CC1_VCONN_EN    5
#define CC2_VCONN_EN    8
// PD Receive channels
#define CC1_RX          6
#define CC2_RX          7
// PD Transmit channels
#define CC1_TX_H        9
#define CC1_TX_L        10
#define CC2_TX_H        11
#define CC2_TX_L        12
// USB PIO (on interrogator port)
#define PIO_USB_DP      13
#define PIO_USB_DM      14
// I2C & Interrupt for FUSB302B (ONLY used for self-test debugging - not populated on most builds)
#define SDA             20
#define SCL             21
#define TCPC_INT        22
// ADC channels for CC lines (these are NOT necessary & typically cut to avoid the RP2 from drawing current from the CC lines before the regulator turns on)
#define CC1_CONN        26
#define CC2_CONN        27

#endif
