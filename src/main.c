/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "bmc.pio.h"

//Define pins (to be used by PIO for BMC TX/RX)
const uint pin_tx = 7;
const uint pin_rx = 6;
uint8_t buf1_count;

void dma_setup(PIO pio, uint sm, uint dma_chan, uint32_t *capture_buf, size_t capture_size_words) {
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_dreq(&c, pio_get_dreq(pio, sm, false));

    dma_channel_configure(dma_chan, &c, 
	    capture_buf,
	    &pio->rxf[sm],
	    capture_size_words,
	    true
    );
}

bool fetch_u32_word(uint32_t *input_buffer, uint8_t *input_offset, uint64_t *output_buffer, uint8_t *output_offset) {
    bool operation = false;
    while(*output_offset >= 32) {		//Return false if output buffer has insufficient space
	//printf("inputbuf: %X\n", input_buffer[*input_offset]);
	if(!input_buffer[*input_offset])	//....or input buffer is empty
	    break;
	//printf("Need to feed process_buf");
	*output_buffer |= input_buffer[*input_offset] << (64 - *output_offset);
	*output_offset -= 32;
	*input_offset++;
	operation |= true;
    }
    return operation;				//Returns true if output buffer refilled; false otherwise
}

int main() {
    // Initialize IO & PIO
    stdio_init_all();
    PIO pio = pio0;
    //Define PIO state machines
    uint sm_tx = 0;
    uint sm_rx = 1;
    //Define DMA channel
    uint dma_chan = 0;
    uint32_t buf1[256];
    for(int i=0;i<=255;i++) {
        buf1[i]=0x00000000;
    }
    //Define processing buffer
    uint64_t procbuf = 0x1230000000004338;
    uint8_t proc_freed_offset = 64;

    /* Initialize TX FIFO
    uint offset_tx = pio_add_program(pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    pio_sm_set_enabled(pio, sm_tx, false);
    pio_sm_put_blocking(pio, sm_tx, 0);
    pio_sm_put_blocking(pio, sm_tx, 0x0ff0a55a);
    pio_sm_put_blocking(pio, sm_tx, 0x12345678);
    pio_sm_set_enabled(pio, sm_tx, true);
    differential_manchester_tx_program_init(pio, sm_tx, offset_tx, pin_tx, 125.f / (5.3333)); */
    
    /* Initialize RX FIFO */
    uint offset_rx = pio_add_program(pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(pio, sm_rx, offset_rx, pin_rx, 125.f / (5.3333));

    /* Initialize DMA for bmc rx */
    dma_setup(pio, sm_rx, dma_chan, buf1, sizeof(&buf1));

    sleep_ms(3000);
 
    while(true) {
	//printf("buf0: 0x%X\nbuf1: 0x%X\nbuf2: 0x%X\nbuf3: 0x%X\n", buf1[0], buf1[1], buf1[2], buf1[3]);
	if(fetch_u32_word(buf1, &buf1_count, &procbuf, &proc_freed_offset))
	    printf("procbuf: 0x%X-%X\nOffset: %d\n", (uint32_t)(procbuf >> 32), (uint32_t)procbuf, proc_freed_offset);
	sleep_ms(2000);
	/*
	if(!pio_sm_is_rx_fifo_empty(pio, sm_rx)) {
            printf("%16d - %08x\n", time_us_32(), pio_sm_get(pio, sm_rx));
	}
	*/
    }
}
