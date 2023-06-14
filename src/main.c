/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "bmc.pio.h"
#include "pdb_msg.h"

#define SM_TX 0
#define SM_RX 1

//Define pins (to be used by PIO for BMC TX/RX)
const uint pin_tx = 7;
const uint pin_rx = 6;
uint8_t buf1_input_count = 0;
uint16_t buf1_output_count;

uint32_t *buf1;
PIO pio = pio0;

void bmc_rx_cb() {
    if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
        //printf("%16d - %08x\n", time_us_32(), pio_sm_get(pio, SM_RX));
	buf1[buf1_input_count] = pio_sm_get(pio, SM_RX);
	buf1_input_count++;
    }
    if(pio_interrupt_get(pio, 0)) {
	pio_interrupt_clear(pio, 0);
    } 
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

	//printf("FETCHDBG: Input wOffset: %u Input bOffset: %u Input: 0x%X\n", input_wordoffset, bitoffset, input_buffer[input_wordoffset]);
	*output_buffer |= ((input_buffer[input_wordoffset] >> bitoffset)	// Offset pre-processing buffer
				& 0xFFFFFFFF >> (32 - bits_to_transfer))	// Mask input bit offset
				<< (32 - *output_bitoffset);			// Shift to output bit offset
	*output_bitoffset -= bits_to_transfer;
	*input_bitoffset += bits_to_transfer;
	return true;
    }
    return false;					// Returns true if output buffer refilled; false otherwise
}
bool debug_u32_word(uint32_t *input_buffer, uint8_t max_num) {
    for(int i = 0; i < (max_num + 1); i++) {
	printf("%3d: %X\n", i, input_buffer[i]);
    }
}

bool bmc_eliminate_preamble(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    if(*freed_bits_procbuf)	// Don't even bother checking if process_buffer is not full
        return false;
    switch(*process_buffer) {
        case (0xAAAAAAAA) :
		//No single shift needed
		break;
	case (0x55555555) :
		//Need to shift 1 bit
		*process_buffer >>= 1;
		*freed_bits_procbuf++;
		break;
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
    //Outputs bits (6..0)
	//bit 4 indicates K-code output
	//bit 5 indicates an error (invalid symbol)
	//bit 6 indicates an error (empty buffer)
    uint8_t ret;
    if(*freed_bits_procbuf > 32 - 5)
	return 0x40;			//Return - empty buffer
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
    return ret;
}
uint8_t bmc_kcode_retrieve(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    int ret = bmc_4b5b_decode(process_buffer, freed_bits_procbuf);
    if(!(ret & 0x20) && (ret & 0x10)) {
	return ret & 0b111;
    } else {
	printf("bmc_kcode_retrieve - Error: Invalid Symbol : 0x%X\n", ret);
    }
}
bool bmc_print_type(uint16_t pd_frame_type) {
    switch (pd_frame_type & 0xFFF) {
	    case (0x200) :
		printf("SOP\n");
		break;
	    case (0x480) :
		printf("SOP_prime\n");
		break;
	    case (0x410) :
		printf("SOP_double_prime\n");
		break;
	    case (0x520) :
		printf("SOP_prime_debug\n");
		break;
	    case (0x2A0) :
		printf("SOP_double_prime_debug\n");
		break;
	    case (0x8DB) :
		printf("Hard Reset\n");
		break;
	    case (0x4C3) :
		printf("Cable Reset\n");
		break;
	    default :
		printf("Error - not a frame type: 0x%X\n", pd_frame_type);
		return false;
    }
    return true;
}
uint16_t pd_uint16_pull(uint32_t *process_buffer, uint8_t *freed_bits_procbuf, int8_t *error_status) {
    uint8_t tmp;
    uint16_t ret = 0;
    for(int i=0;i<4;i++) {
	tmp = bmc_4b5b_decode(process_buffer, freed_bits_procbuf);
	switch (tmp & 0xF0) {
	    case (0x10) :// K-Code symbol
		*error_status = 1 << 0;
		printf("Error: pd_uint16_pull: Unexpected K-Code symbol received. - 0x%X\n", tmp);
		return ret;
		break;
	    case (0x20) :// Invalid Symbol
		*error_status = 1 << 1;
		printf("Error: pd_uint16_pull: Invalid 4b5b symbol received. - 0x%X\n", tmp);
		return ret;
		break;
	    case (0x40) :// Empty Buffer
		*error_status = 1 << 2;
		printf("Error: pd_uint16_pull: Empty 4b5b process buffer. - 0x%X\n", tmp);
		return ret;
		break;
	    default :	 // Hex data (no error)
		ret |= ((tmp & 0xF) << i * 4);
		break;
	}
    }
    return ret;
}
/*
uint32_t pd_uint32_pull(uint32_t *process_buffer, uint8_t *freed_bits_procbuf, int8_t *error_status) {
    uint8_t tmp;
    uint32_t ret = 0;
    for(int i=0;i<8;i++) {
	tmp = bmc_4b5b_decode(process_buffer, freed_bits_procbuf);
	//TODO - add error detection logic
	if(tmp == 0x17) {	//EOP
	    *error_status = 1;
	    return ret;
	} else { 
	    ret |= ((tmp & 0xF) << i * 4);
	}
    }
    return ret;
}
*/
uint8_t pd_burst_pull(uint32_t *process_buffer, uint8_t *freed_bits_procbuf, int8_t *error_status, uint8_t *data_buf, uint8_t max_bytes) {//Returns # of received bytes (excluding EOP)
    uint8_t tmp;
    uint32_t ret = 0;
    for(int i=0;i<(max_bytes*2-1);i++) {
	tmp = bmc_4b5b_decode(process_buffer, freed_bits_procbuf);
	//TODO - add error detection logic
	if(tmp == 0x17) {	//EOP
	    *error_status = 1;
	    return i;
	} else { 
	    data_buf[i] = (tmp & 0xF) << (i & 0x1);
	}
    }
    return ret;
}
int main() {
    // Initialize IO & PIO
    stdio_init_all();
    buf1 = malloc(256 * 4);
    if(buf1 == NULL) 
	    printf("Error - buf1 is a NULL pointer.");
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
    int8_t bmc_err_status; //1 = EOP, negative val = error
    pd_msg lastmsg;

    /* Initialize TX FIFO
    uint offset_tx = pio_add_program(pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    pio_sm_set_enabled(pio, SM_TX, false);
    pio_sm_put_blocking(pio, SM_TX, 0);
    pio_sm_put_blocking(pio, SM_TX, 0x0ff0a55a);
    pio_sm_put_blocking(pio, SM_TX, 0x12345678);
    pio_sm_set_enabled(pio, SM_TX, true);
    differential_manchester_tx_program_init(pio, SM_TX, offset_tx, pin_tx, 125.f / (5.3333)); */
    
    /* Initialize RX FIFO */
    uint offset_rx = pio_add_program(pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(pio, SM_RX, offset_rx, pin_rx, 125.f / (5.3333));

    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, bmc_rx_cb);
    irq_set_enabled(PIO0_IRQ_0, true);
 
    // Debug message
    sleep_ms(13000);
    debug_u32_word(buf1, 255);//255

    while(true) {
	fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
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
		    pd_frame_type |= bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 3;
		    proc_state++;
		    break;
		case (3) ://Ordered Set 3
		    pd_frame_type |= bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 6;
		    proc_state++;
		    break;
		case (4) ://Ordered Set 4
		    pd_frame_type |= bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 9;
		    bmc_print_type(pd_frame_type);
		    proc_state++;
		    break;
		case (5) ://Data acquisition stage
		    lastmsg.hdr = pd_uint16_pull(&procbuf, &proc_freed_offset, &bmc_err_status);
		    fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);//TODO - remove these
		    fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		    if(proc_freed_offset) {
			proc_state = 6;//Stalled
			break;
		    }
		    printf("Header: %4X\n", lastmsg.hdr);
		    // Retrieve each Object value (if available)
		    for(uint i=0;i < ((lastmsg.hdr >> 12) & 0b111);i++) { //TODO: Implement NumDataObjects macro
		        fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);//TODO - remove these
		        fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		        lastmsg.obj[i] = pd_uint16_pull(&procbuf, &proc_freed_offset, &bmc_err_status);
		        fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		        fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		        lastmsg.obj[i] |= pd_uint16_pull(&procbuf, &proc_freed_offset, &bmc_err_status) << 16;
		        printf("Obj%u: %8X\n", i, lastmsg.obj[i]);
		    }
		    // Retrieve CRC (we don't currently do anything with it - TODO)
		    fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);//TODO - remove these
		    fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		    pd_uint16_pull(&procbuf, &proc_freed_offset, &bmc_err_status);
		    fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		    fetch_u32_word(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
		    pd_uint16_pull(&procbuf, &proc_freed_offset, &bmc_err_status);// << 16;
		    // Attempt to retrieve EOP - TODO
		    //
		    //
		    proc_state++;
		    break;
		case (6) ://Data stall stage - TODO
		    break;
		case (7) ://EOP received - TODO
		    break;
	}
	sleep_ms(100);
	/*
	if(bmc_eliminate_preamble(&procbuf, &proc_freed_offset)) {
	    
	    printf("procbuf: 0x%X\nOffset: %d\n", procbuf, proc_freed_offset);
	}
	sleep_ms(2000);
	if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
            printf("%16d - %08x\n", time_us_32(), pio_sm_get(pio, SM_RX));
	}
	*/
    }
}
