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
uint16_t buf1_count;

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

bool fetch_u32_word(uint32_t *input_buffer, uint16_t *input_bitoffset, uint32_t *output_buffer, uint8_t *output_bitoffset) {
    uint8_t input_wordoffset, bitoffset;				// Basically - all of this logic 
    if(!*input_bitoffset) {						// ensures that the bitwise 
        input_wordoffset = 0;						// logic below does not attempt 
	bitoffset = 0;							// to divide by zero.
    } else {								//
        input_wordoffset = (*input_bitoffset / 32);			//
	bitoffset = (*input_bitoffset % 32);				//
    }									//

    if(*output_bitoffset 						// Ensure output buffer has sufficient space 
		    && input_buffer[input_wordoffset]) {		// and input buffer is not empty

	uint8_t bits_to_transfer = *output_bitoffset;
	if((32 - bitoffset) < *output_bitoffset) {	// If more output bits are needed then input bits are available..
		bits_to_transfer = (32 - bitoffset);	// ..transfer as many as available. (Without introducing extra zeros)
	}						// Otherwise - transfer as many as needed by output buffer.

	*output_buffer |= ((input_buffer[input_wordoffset] >> bitoffset)	// Offset pre-processing buffer
				& 0xFFFFFFFF << (32 - bits_to_transfer))	// Mask input bit offset
				<< (32 - *output_bitoffset);			// Shift to output bit offset
	*output_bitoffset -= bits_to_transfer;
	*input_bitoffset += bits_to_transfer;
	return true;
    }
    return false;					// Returns true if output buffer refilled; false otherwise
}

bool bmc_eliminate_preamble(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    if(*freed_bits_procbuf)	// Don't even bother checking if process_buffer is not full
        return false;
    switch(*process_buffer) {
        case (0xAAAAAAAA) :
		//No shifting needed
		break;
	case (0x55555555) :
		//Need to shift
		*process_buffer >>= 1;
		*freed_bits_procbuf++;
		break;
	default :
		*process_buffer >>= 4;
    		*freed_bits_procbuf += 4;
		return false;
    }

    while(((*process_buffer & 0b11) == 0b10) && (*freed_bits_procbuf != 32)) {
        *process_buffer >>= 2;
	*freed_bits_procbuf += 2;
    }
    switch(*process_buffer & 0b11111) {		// Return true if Sync1 or Reset1 symbol is found
        case (0b11000) :// Sync1
	case (0b00111) :// Reset1
	    return true;
	    break;
    }
    return false;				//Otherwise return false
}
uint8_t bmc_4b5b_decode(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    //Outputs bits (5..0)
	//bit 4 indicates K-code output
	//bit 5 indicates an error (invalid symbol)
    uint8_t ret;
    switch(*process_buffer & 0b11111) {
	case (0b11110) :// 0x0
	    ret = 0x0;
	    break;
	case (0b01001) :// 0x1
	    ret = 0x1;
	    break;
	case (0b10100) :// 0x2
	    ret = 0x2;
	    break;
	case (0b10101) :// 0x3
	    ret = 0x3;
	    break;
	case (0b01010) :// 0x4
	    ret = 0x4;
	    break;
	case (0b01011) :// 0x5
	    ret = 0x5;
	    break;
	case (0b01110) :// 0x6
	    ret = 0x6;
	    break;
	case (0b01111) :// 0x7
	    ret = 0x7;
	    break;
	case (0b10010) :// 0x8
	    ret = 0x8;
	    break;
	case (0b10011) :// 0x9
	    ret = 0x9;
	    break;
	case (0b10110) :// 0xA
	    ret = 0xA;
	    break;
	case (0b10111) :// 0xB
	    ret = 0xB;
	    break;
	case (0b11010) :// 0xC
	    ret = 0xC;
	    break;
	case (0b11011) :// 0xD
	    ret = 0xD;
	    break;
	case (0b11100) :// 0xE
	    ret = 0xE;
	    break;
	case (0b11101) :// 0xF
	    ret = 0xF;
	    break;
	case (0b11000) :// K-code Sync-1
	    ret = 0x10;
	    break;
	case (0b10001) :// K-code Sync-2
	    ret = 0x11;
	    break;
	case (0b00110) :// K-code Sync-3
	    ret = 0x12;
	    break;
	case (0b00111) :// K-code RST-1
	    ret = 0x13;
	    break;
	case (0b11001) :// K-code RST-2
	    ret = 0x14;
	    break;
	case (0b01101) :// K-code EOP
	    ret = 0x17;
	    break;
	default :	// Error - Invalid symbol
	    ret = 0x20;
    }
    *process_buffer >>= 5;
    *freed_bits_procbuf += 5;
}
uint8_t bmc_kcode_retrieve(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    int ret = bmc_4b5b_decode(process_buffer, freed_bits_procbuf);
    if(!(ret & 0x20) && (ret & 0x10)) {
	return ret & 0b111;
    } else {
	printf("bmc_kcode_retrieve - Error: Invalid Symbol : 0x%X\n", ret);
    }
}
void bmc_print_type(uint16_t pd_frame_type) {
    if(!(pd_frame_type & 0b10000)) {
	printf("Error - not a frame type.\n");
	return;
    }
    switch (pd_frame_type & 0b1111) {
	    case (0x1000) :
		printf("SOP\n");
		break;
	    case (0x2200) :
		printf("SOP_prime\n");
		break;
	    case (0x2020) :
		printf("SOP_double_prime\n");
		break;
	    case (0x2440) :
		printf("SOP_prime_debug\n");
		break;
	    case (0x1240) :
		printf("SOP_double_prime_debug\n");
		break;
	    case (0x4333) :
		printf("Hard Reset\n");
		break;
	    case (0x2303) :
		printf("Cable Reset\n");
		break;
    }
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
    uint32_t procbuf = 0x00000000;
    uint8_t proc_freed_offset = 32;
    //Define processing state variable
    uint8_t proc_state = 0;	// 0 = Preamble stage
    				// 1 = Ordered Set 1
				// 2 = Ordered Set 2
				// 3 = Ordered Set 3
				// 4 = Ordered Set 4 - determine frame type
				// 5 = Data
				// 6 = Data (Stalled) - awaiting more data
				// 7 = EOP received - parse data

    uint16_t pd_frame_type;

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
	//printf("Debugproc: 0x%X\nDebugoffset: %d\n", procbuf, proc_freed_offset);
	//fetch_u32_word(buf1, &buf1_count, &procbuf, &proc_freed_offset);
	switch(proc_state) {
		case (0) ://Preamble stage
		    if(bmc_eliminate_preamble(&procbuf, &proc_freed_offset)) {
			printf("Buf: 0x%X\n", procbuf);
			proc_state++;
		    }
		    break;
		case (1) ://Ordered Set 1
		    pd_frame_type = bmc_kcode_retrieve(&procbuf, &proc_freed_offset);
		    proc_state++;
		    break;
		case (2) ://Ordered Set 2
		    pd_frame_type = bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 3;
		    proc_state++;
		    break;
		case (3) ://Ordered Set 3
		    pd_frame_type = bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 6;
		    proc_state++;
		    break;
		case (4) ://Ordered Set 4
		    pd_frame_type = bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 9;
		    bmc_print_type(pd_frame_type);
		    proc_state++;
		    break;
	}
	if(bmc_eliminate_preamble(&procbuf, &proc_freed_offset)) {
	    
	    printf("procbuf: 0x%X\nOffset: %d\n", procbuf, proc_freed_offset);
	}
	sleep_ms(2000);
	/*
	if(!pio_sm_is_rx_fifo_empty(pio, sm_rx)) {
            printf("%16d - %08x\n", time_us_32(), pio_sm_get(pio, sm_rx));
	}
	*/
    }
}
