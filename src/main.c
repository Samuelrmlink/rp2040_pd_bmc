/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "bmc.pio.h"

//Define pins (to be used by PIO for BMC TX/RX)
const uint pin_tx = 7;
const uint pin_rx = 6;

int main() {
    // Initialize IO & PIO
    stdio_init_all();
    PIO pio = pio0;
    //Define PIO state machines
    uint sm_tx = 0;
    uint sm_rx = 1;

    /* Initialize TX FIFO
    uint offset_tx = pio_add_program(pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    pio_sm_set_enabled(pio, sm_tx, false);
    pio_sm_put_blocking(pio, sm_tx, 0);
    pio_sm_put_blocking(pio, sm_tx, 0x0ff0a55a);
    pio_sm_put_blocking(pio, sm_tx, 0x12345678);
    pio_sm_set_enabled(pio, sm_tx, true);
    differential_manchester_tx_program_init(pio, sm_tx, offset_tx, pin_tx, 125.f / (5.3333));
    */
    
    /* Initialize RX FIFO */
    uint offset_rx = pio_add_program(pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(pio, sm_rx, offset_rx, pin_rx, 125.f / (5.3333));

    sleep_ms(3000);
 
    while(true)
        printf("%08x\n", pio_sm_get_blocking(pio, sm_rx));
}
