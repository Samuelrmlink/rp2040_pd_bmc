/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "main_i.h"

#define SM_TX 0
#define SM_RX 1

//Define pins (to be used by PIO for BMC TX/RX)
const uint pin_tx = 9;
const uint pin_rx = 6;
//uint8_t buf1_input_count = 0;
//uint16_t buf1_output_count;
//bool buf1_rollover = false; // Indicates when inputbuf has rolled over (reset once the outputbuf also rolls over)

uint32_t *buf1;
PIO pio = pio0;

bool bmc_check_during_operation = true;

//uint32_t us_since_last_u32;

bmcDecode* bmc_d;

void bmc_rx_check() {
    if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
        //printf("%16d - %08x\n", time_us_32(), pio_sm_get(pio, SM_RX));
	bmc_d->inBuf = pio_sm_get(pio, SM_RX);
	bmc_d->rxTime = time_us_32();
	//if(buf1_input_count == 255) {		//Set rollover (flag for output buffer logic)
	//    buf1_rollover = true;
	//}
	//buf1_input_count++;
    }
}
void bmc_rx_cb() {
    bmc_rx_check();
    if(pio_interrupt_get(pio, 0)) {
	pio_interrupt_clear(pio, 0);
    } 
}
/*
bool fetch_u32_word(uint32_t *input_buffer, uint16_t *input_bitoffset, uint32_t *output_buffer, uint8_t *output_bitoffset) {
    if(bmc_check_during_operation) bmc_rx_check();
    uint8_t input_wordoffset, bitoffset;
    input_wordoffset = (*input_bitoffset / 32);
    bitoffset = (*input_bitoffset % 32);
    if(*output_bitoffset 						// Ensure output buffer has sufficient space 
		    && input_buffer[input_wordoffset]) {		// and input buffer is not empty

	uint8_t bits_to_transfer = *output_bitoffset;
	if((32 - bitoffset) < *output_bitoffset) {	// If more output bits are needed then input bits are available..
		bits_to_transfer = (32 - bitoffset);	// ..transfer as many as available. (Without introducing extra zeros)
	}						// Otherwise - transfer as many as needed by output buffer.

	if(*input_bitoffset + bits_to_transfer > 256 * 32) {
	    bits_to_transfer -= *input_bitoffset + bits_to_transfer - 256 * 32;
	}
	*output_buffer |= ((input_buffer[input_wordoffset] >> bitoffset)	// Offset pre-processing buffer
				& 0xFFFFFFFF >> (32 - bits_to_transfer))	// Mask input bit offset
				<< (32 - *output_bitoffset);			// Shift to output bit offset
	*output_bitoffset -= bits_to_transfer;
	*input_bitoffset += bits_to_transfer;
	return true;
    }
    return false;					// Returns true if output buffer refilled; false otherwise
}
bool bmc_data_available(uint8_t num_bits_requested, uint32_t *input_buffer, uint16_t *input_bitoffset, uint32_t *output_buffer, uint8_t *output_bitoffset, bool procbuf_only) {
    uint16_t num_bits_available;
    uint8_t num_words_added;
    uint8_t additional_words;

    if(buf1_rollover && *input_bitoffset >= 256 * 32) {
	*input_bitoffset = 0;
	buf1_rollover = false;
    }
    if(buf1_rollover && !procbuf_only) {
	num_words_added = 255;
	additional_words = buf1_input_count;//TODO -fix
    } else {
	num_words_added = (buf1_input_count - 1);
	additional_words = 0;
    }
	num_bits_available = ((num_words_added		      //   # (whole) words added
		- (*input_bitoffset / 32))	      	      // - # (whole) words consumed
		//-------------------------------------------------------------------------
		* 32)					      // * 32 bits/word
		//=========================================================================
		+ (32 - (*input_bitoffset % 32 + 1))	      // + # remainder bits (in current word of pre-procbuf)
		+ (32 - *output_bitoffset)		      // + # remainder bits (procbuf)
		+ (32 * additional_words);		      // + # additional 32-bit words (after rolled-over value - only used when [rollover == true])

    fetch_u32_word(input_buffer, input_bitoffset, output_buffer, output_bitoffset);
    fetch_u32_word(input_buffer, input_bitoffset, output_buffer, output_bitoffset);
    if(num_bits_available >= num_bits_requested) {
        return true;
    } else { 
	return false;
    }
}
bool fetch_u32_word_safe(uint32_t *input_buffer, uint16_t *input_bitoffset, uint32_t *output_buffer, uint8_t *output_bitoffset) {
    return bmc_data_available(40, input_buffer, input_bitoffset, output_buffer, output_bitoffset, true);
}
bool debug_u32_word(uint32_t *input_buffer, uint8_t max_num) {
    for(int i = 0; i < (max_num + 1); i++) {
	printf("%3d: %X\n", i, input_buffer[i]);
    }
}
*/
bool bmc_eliminate_preamble_stage1(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    if(*freed_bits_procbuf)
	return false;
    bool possible_preamble_found = false;
    uint8_t preamble_offset = 0;
    for(int i=0;i<=24;i++) {
        switch((*process_buffer >> i) & (0xFF >> i)) {
	    case (0x55) :
	   	 preamble_offset++;
	    case (0xAA) :
	   	 possible_preamble_found = true;
	    	break;
        }
        if(possible_preamble_found) {
	    break;
        }
    }
    if(!possible_preamble_found) {
	preamble_offset = 32;
    }
    *process_buffer >>= preamble_offset;
    *freed_bits_procbuf += preamble_offset;
    return possible_preamble_found;
}
bool bmc_eliminate_preamble_stage2(uint32_t *process_buffer, uint8_t *freed_bits_procbuf) {
    while(*freed_bits_procbuf <= 30 && (*process_buffer & 0b11) == 0b10) {
	*freed_bits_procbuf += 2;
	*process_buffer >>= 2;
    }
}
int8_t bmc_locate_ordered_set(uint32_t *process_buffer, uint8_t *freed_bits_procbuf, bool *preamble_lock) {
    int8_t ret = 0;
    if(!(*preamble_lock)) {
    	*preamble_lock = bmc_eliminate_preamble_stage1(process_buffer, freed_bits_procbuf);
    }
    if(*preamble_lock) {
	bmc_eliminate_preamble_stage2(process_buffer, freed_bits_procbuf);
    }
    if(*freed_bits_procbuf <= 12 && *preamble_lock && ((*process_buffer & 0xFFFFF) != 0xAAAAA)) {
	int8_t ret = 0;
	switch(*process_buffer & 0xFFFFF) {
	    case (0b11001001110011100111) : // Hard Reset
		ret = 1;
		break;
	    case (0b00110001111100000111) : // Cable Reset
		ret = 2;
		break;
	    case (0b10001110001100011000) : // SOP
		ret = 3;
		break;
	    case (0b00110001101100011000) : // SOP'
		ret = 4;
		break;
	    case (0b00110110000011011000) : // SOP''
		ret = 5;
		break;
	    case (0b00110110011100111000) : // SOP' Debug
		ret = 6;
		break;
	    case (0b10001001101100111000) : // SOP'' Debug
		ret = 7;
		break;
	}
    	if(ret) {
		*process_buffer >>= 20;
		*freed_bits_procbuf += 20;
		*preamble_lock = false;
        	return ret;
    	}
    }
    return 0;
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
/*
uint32_t pd_bytes_to_reg(uint32_t *preproc_buf, uint16_t *preproc_offset, uint32_t *proc_buf, uint8_t *proc_offset, int8_t *error_status, uint8_t num_bytes) {
    //Initialize temporary variables
    uint8_t tmp;
    uint32_t ret = 0;
    bool debug = false;

    //Figure out how many raw (PHY Layer - 4b5b symbols) bits we'll have to read
    uint8_t num_bits_required;
    if(!num_bytes) {
	num_bits_required = 5;			// 1 symbol
    } else {
	num_bits_required = 10 * num_bytes;     // 2 symbols per byte
    }
    
    //Check whether we have enough bits (totalled between all buffers)
    if(bmc_data_available(num_bits_required, preproc_buf, preproc_offset, proc_buf, proc_offset, false)) {
	for(int i=0;i<num_bits_required/5;i++) {
	    if(debug) printf("before: %X-%u  %X-%u-%u r%u   ", *proc_buf, *proc_offset, preproc_buf[*preproc_offset / 32], *preproc_offset/32, *preproc_offset % 32, buf1_rollover);
	    tmp = bmc_4b5b_decode(proc_buf, proc_offset);
	    if(debug) printf("afterdecode: %X-%u\n", *proc_buf, *proc_offset);

	    fetch_u32_word_safe(preproc_buf, preproc_offset, proc_buf, proc_offset);
	    if(tmp & 0xF0) {
		break;
	    }
	    ret |= ((tmp & 0xF) << i * 4);
	    if(debug) printf("bmc_decoded: %X:%u ", tmp, i * 4);
	}
    } else {
	*error_status = -2;
	return ret;
    }

    //Check for EOP/errors
    switch (tmp & 0xF0) {
	case (0x10) :// K-Code symbol
	    if(tmp == 0x17) {
	    	if(num_bytes) { //Only triggers error if EOP is unexpected (silence when EOP is expected)
		    *error_status = 1;
	    	    printf("Error: pd_bytes_to_reg: Unexpected EOP symbol received. - 0x%X : 0x%X\n", tmp, ret);
		} else {
		    ret = 0xFF;
		}
	    } else {
		*error_status = -3;
	    	printf("Error: pd_bytes_to_reg: Unexpected K-Code symbol received. - 0x%X : 0x%X\n", tmp, ret);
	    }
	    break;
	case (0x20) :// Invalid Symbol
	    *error_status = -1;
	    printf("Error: pd_bytes_to_reg: Invalid 4b5b symbol received. - 0x%X : 0x%X\n", tmp, ret);
	    break;
	case (0x40) :// Empty Buffer
	    *error_status = -2;
//	    printf("Error: pd_bytes_to_reg: Empty 4b5b process buffer. - 0x%X\n", tmp);
	    break;
    }
    return ret;
}
*/
bool pd_read_error_handler(int8_t *error_code, uint8_t *process_state) {
    bool ret = true;	//Default state - true (meaning there are errors)
    switch (*error_code) {
	case (1) :		// Unexpected EOP
	    printf("EOP unexpected: %u\n", *process_state); //TODO: DEBUGG Remove!
	case (-1) :		// Invalid symbol and/or frame: 
	    //TODO - stash away somewhere
	    *process_state = 0;
	    *error_code = 0;
	    break;
	case (-2) :		// Empty buffer (insufficient space - not necessarily empty)
	    *error_code = 0;
	    break;
	case (-3) :
	    //TODO - implement unexpected K-Code handler
	    //Reset (for now...)
	    *process_state = 0;
	    *error_code = 0;
	    break;
	case (0) :		//No error
	default :		//& catch-all
	    ret = false;
    }
    return ret;
}
/*
void bmc_4b_to_5b(uint8_t input_symbol, uint8_t *output_symbol) {
    switch (input_symbol) {
	case (0x00) :// 0x0
	    *output_symbol = 0b11110;
	    break;
	case (0x01) :// 0x1
	    *output_symbol = 0b01001;
	    break;
	case (0x02) :// 0x2
	    *output_symbol = 0b10100;
	    break;
	case (0x03) :// 0x3
	    *output_symbol = 0b10101;
	    break;
	case (0x04) :// 0x4
	    *output_symbol = 0b01010;
	    break;
	case (0x05) :// 0x5
	    *output_symbol = 0b01011;
	    break;
	case (0x06) :// 0x6
	    *output_symbol = 0b01110;
	    break;
	case (0x07) :// 0x7
	    *output_symbol = 0b01111;
	    break;
	case (0x08) :// 0x8
	    *output_symbol = 0b10010;
	    break;
	case (0x09) :// 0x9
	    *output_symbol = 0b10011;
	    break;
	case (0x0A) :// 0xA
	    *output_symbol = 0b10110;
	    break;
	case (0x0B) :// 0xB
	    *output_symbol = 0b10111;
	    break;
	case (0x0C) :// 0xC
	    *output_symbol = 0b11010;
	    break;
	case (0x0D) :// 0xD
	    *output_symbol = 0b11011;
	    break;
	case (0x0E) :// 0xE
	    *output_symbol = 0b11100;
	    break;
	case (0x0F) :// 0xF
	    *output_symbol = 0b11101;
	    break;
	case (0x10) :// K-code Sync-1
	    *output_symbol = 0b11000;
	    break;
	case (0x11) :// K-code Sync-2
	    *output_symbol = 0b10001;
	    break;
	case (0x12) :// K-code Sync-3
	    *output_symbol = 0b00110;
	    break;
	case (0x13) :// K-code RST-2
	    *output_symbol = 0b00111;
	    break;
	case (0x14) :// K-code RST-2
	    *output_symbol = 0b11001;
	    break;
	case (0x17) :// K-code EOP
	    *output_symbol = 0b01101;
	    break;
	default :
	    //Error - TODO
    }
}
void bmc_4b5b_encode(uint8_t input_symbol, uint32_t *output_buffer, uint16_t *output_offset) {
    uint8_t encoded_symbol;
    uint8_t bitshift = 32 - *output_offset % 32;
    if(bitshift > 5) bitshift = 5;	// Ensure bitshift is always 5 or less
    bmc_4b_to_5b(input_symbol, &encoded_symbol);
    if(!(*output_offset % 32)) output_buffer[*output_offset / 32] = 0;		//Ensure output buffer is clean (if this is the first time we are writing to it)
    output_buffer[*output_offset / 32] |= (encoded_symbol & 0b11111 >> 5 - bitshift) << *output_offset % 32;
    *output_offset += bitshift;
    if(bitshift < 5) {
	encoded_symbol >>= 4 - bitshift;
	output_buffer[*output_offset / 32] = (encoded_symbol & 0b11111 >> bitshift); //Ensure output buffer is clean
	*output_offset += 5 - bitshift;
    }
}
void pd_respond_goodcrc(void *rxd) {
    pd_msg *msg_rxd = rxd;
    pd_msg msg_resp;
    uint32_t crc;

    uint8_t msg_id = (msg_rxd->hdr >> 9) & 0b111;
    //uint8_t spec_rev = (msg_rxd->hdr >> 6) & 0b11;
    uint8_t spec_rev = 0b01;

    msg_resp.hdr = msg_id << 9 | spec_rev << 6 | 0b00001;
    crc = crc32_pdmsg(msg_resp.bytes);

    uint32_t data_buf[5];
    data_buf[0] = 0x55555555;
    data_buf[1] = 0x55555555;
    data_buf[2] = 0x00000155;
    uint16_t offset = 74;
    bmc_4b5b_encode(0x10, data_buf, &offset);//Sync-1
    bmc_4b5b_encode(0x10, data_buf, &offset);//Sync-1
    bmc_4b5b_encode(0x10, data_buf, &offset);//Sync-1
    bmc_4b5b_encode(0x11, data_buf, &offset);//Sync-2
    for(int i=0;i<4;i++) {
    	bmc_4b5b_encode(0xF & (msg_resp.hdr >> 4 * i), data_buf, &offset);
    }
    for(int i=0;i<8;i++) {
    	bmc_4b5b_encode(0xF & (crc >> 4 * i), data_buf, &offset);
    }
    bmc_4b5b_encode(0x17, data_buf, &offset);//EOP
    printf("GOODCRC resp: %X:%X:%X-%X-%X-%X-%X\n", msg_resp.hdr, crc, data_buf[0], data_buf[1], data_buf[2], data_buf[3], data_buf[4]);
    gpio_put(8, 1);
    pio_sm_put_blocking(pio, SM_TX, data_buf[0]);
    pio_sm_put_blocking(pio, SM_TX, data_buf[1]);
    pio_sm_put_blocking(pio, SM_TX, data_buf[2]);
    pio_sm_put_blocking(pio, SM_TX, data_buf[3]);
    pio_sm_put_blocking(pio, SM_TX, data_buf[4]);
    pio_sm_set_enabled(pio, SM_TX, true);
    printf("pio_sm_get_tx_fifo_level: %X\n", pio_sm_get_tx_fifo_level(pio, SM_TX));
    while(!pio_sm_is_tx_fifo_empty(pio, SM_TX)) sleep_us(5);
    sleep_us(195);
    gpio_put(8, 0);
}
*/
int main() {
    // Initialize IO & PIO
    stdio_init_all();
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);
    buf1 = malloc(256 * 4);
    if(buf1 == NULL) 
	    printf("Error - buf1 is a NULL pointer.");
    for(int i=0;i<=255;i++) {
        buf1[i]=0x00000000;
    }

    // Allocate BMC decoding struct
    bmc_d = malloc(sizeof(bmcDecode));

    /*
    //Define processing buffer
    uint32_t procbuf = 0x00000000;
    uint8_t proc_freed_offset = 32;
    //Define processing state variable
    uint8_t proc_state = 0;	// 0 = Preamble stage
    				// 1 = Ordered Set
				// 2 = PD Header
				// 3 = Extended Header
				// 4 = Data Objects
				// 5 = CRC
				// 6 = EOP

    uint16_t pd_frame_type;
    uint16_t tmp_uint;
    int8_t bmc_err_status; //1 = EOP, negative val = error
    pd_msg lastmsg;
    epr_pd_msg lastmsg_ext;

    bool preamble_lock = false;
    */

    /* Initialize TX FIFO*/
    uint offset_tx = pio_add_program(pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    differential_manchester_tx_program_init(pio, SM_TX, offset_tx, pin_tx, 125.f / (5.3333));
    pio_sm_set_enabled(pio, SM_TX, false);
    /*
    pio_sm_put_blocking(pio, SM_TX, 0);
    pio_sm_put_blocking(pio, SM_TX, 0x0ff0a55a);
    pio_sm_put_blocking(pio, SM_TX, 0x12345678);
    pio_sm_set_enabled(pio, SM_TX, true);*/
    
    /* Initialize RX FIFO */
    uint offset_rx = pio_add_program(pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(pio, SM_RX, offset_rx, pin_rx, 125.f / (5.3333));

    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, bmc_rx_cb);
    irq_set_enabled(PIO0_IRQ_0, true);
    bmc_check_during_operation = false;		// Override - disables this check
 
//TODO - remove
/*
    buf1_input_count = 250;
    buf1_output_count = (32 * 250);
    bmc_fill2();
*/
/*
    // Debug message
    sleep_ms(4000);
    debug_u32_word(buf1, 255);//255
*/

    bool debug = false;
    uint32_t us_prev = time_us_32();
    uint32_t us_lag, us_current, us_lag_record = 0;

    bool transmitted = false;

/**/
    uint32_t last_usval;
    uint32_t tmpval;
    pd_msg lastmsg;
    bmc_d->pOffset = 0;
    bmc_d->procStage = 0;
    bmc_d->procBuf = 0;

    /* TEST CASE */
    sleep_ms(4);
    bmc_d->inBuf = 0xAAAAAAAA;
    bmc_d->rxTime = time_us_32();
    bmcProcessSymbols(bmc_d, &lastmsg);
    printf("DBGusPassed0: %X, %X, %X, %X\n", bmc_d->inBuf, lastmsg._pad1[0], bmc_d->pOffset, bmc_d->procStage);
    bmc_d->inBuf = 0xAAAAAAAA;
    bmcProcessSymbols(bmc_d, &lastmsg);
    printf("DBGusPassed1: %X, %X, %X, %X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage);
    bmc_d->inBuf = 0x4C6C62AA;
    bmcProcessSymbols(bmc_d, &lastmsg);
    printf("DBGusPassed2: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);


    printf("procStage: %X\n", bmc_d->procStage);
    sleep_ms(3);





    while(true) {
	if(bmc_d->rxTime != last_usval) {
	    tmpval = bmc_d->rxTime - last_usval;
	    last_usval = bmc_d->rxTime;

	    bmcProcessSymbols(bmc_d, &lastmsg);
	    printf("usPassed: %u, %X, %X, %X, %X\n", tmpval, bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage);
	    //printf("us_since_val: %utv: %uval: %Xlastmsg_pad1:%ups:%uprocBuf:%X\n", time_us_32() - bmc_d->rxTime, tmpval, bmc_d->inBuf, lastmsg._pad1[0], decode->procStage, bmc_d->procBuf);
	}

	//Clear the CDC-ACM output
	if(time_us_32() - last_usval > 200) {
	    sleep_us(3);
	}
	//printf("lastusval: %u   us: %u   tmpval: %u\n", last_usval, time_us_32() - last_usval, tmpval);
    }
/*
    while(true) {
	// Delay running loop if actively receiving data
	while(time_us_32() < us_since_last_u32 + 150) {
	    if (bmc_check_during_operation) bmc_rx_check();
	    sleep_us(20);
	}

	// Measure & display number of micro-seconds between runs (for Debugging)
	us_current = time_us_32();
	us_lag = us_current - us_prev;
	us_prev = us_current;
	if(us_lag > us_lag_record) {
		us_lag_record = us_lag;
		printf("us_lag_record: %u\n", us_lag_record);
	}

	// TX Debug testing
*/
	/*
	if(((lastmsg.hdr >> 12) & 0b111) && ((lastmsg.hdr & 0b11111) == 0b00001) && !transmitted && time_us_32() > 10000000) {
	    pd_respond_goodcrc(&lastmsg);
	    transmitted = true;
	}
	*/
	/*
	if(!transmitted && time_us_32() > 10000000) {
	    pd_replay_test1();
	    transmitted = true;
	}
	*/
/*

	if (bmc_check_during_operation) bmc_rx_check();
	fetch_u32_word_safe(buf1, &buf1_output_count, &procbuf, &proc_freed_offset);
	int8_t ordered_set;
	uint32_t crc32_val;
	switch(proc_state & 0xF) {
		case (0) ://Preamble stage
		    ordered_set = bmc_locate_ordered_set(&procbuf, &proc_freed_offset, &preamble_lock);
		    if(ordered_set > 0) {
		    	printf("Ordered set# %d\n", ordered_set);
		    	proc_state += 2;
		    } else if(ordered_set < 0) {
			//printf("Invalid ordered set. %d\n", ordered_set);
			ordered_set = 0;
		    }
		    break;
		case (1) ://Ordered Set
		    pd_frame_type = bmc_kcode_retrieve(&procbuf, &proc_freed_offset) | 
				bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 3 | 
				bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 6 | 
				bmc_kcode_retrieve(&procbuf, &proc_freed_offset) << 9;
		    bmc_print_type(pd_frame_type);
		    proc_state++;
		    break;
		case (2) ://PD Header
		    tmp_uint = pd_bytes_to_reg(buf1, &buf1_output_count, &procbuf, &proc_freed_offset, &bmc_err_status, 2);
		    if(!pd_read_error_handler(&bmc_err_status, &proc_state)) {
		    	printf("Header: %4X\n", tmp_uint);
			proc_state++;
		    }
		    break;
		case (3) ://Extended Header (if applicable)
		    if(tmp_uint >> 15) {	//Extended Header
			lastmsg_ext.hdr = tmp_uint;
			lastmsg_ext.exthdr = pd_bytes_to_reg(buf1, &buf1_output_count, &procbuf, &proc_freed_offset, &bmc_err_status, 2);
			if(!pd_read_error_handler(&bmc_err_status, &proc_state)) {
			    printf("Extended Header: %4X\n", lastmsg_ext.exthdr);
			    proc_state++;
			}
		    } else {			//No Extended Header
			lastmsg.hdr = tmp_uint;
			proc_state++;	//Manually increment [since we aren't running the pd_read_state_handler()]
		    }
		    break;
		case (4) ://Data Objects (if applicable)
		    if(!((lastmsg.hdr >> 12) & 0b111)) { //Skip if not applicable
			proc_state++;
			break;
		    }
		    // Retrieve each Object value (if available)
		    for(uint8_t i = proc_state >> 4;i < ((lastmsg.hdr >> 12) & 0b111);i++) { //TODO: Implement NumDataObjects macro
		        lastmsg.obj[i] = pd_bytes_to_reg(buf1, &buf1_output_count, &procbuf, &proc_freed_offset, &bmc_err_status, 4);
			if(!pd_read_error_handler(&bmc_err_status, &proc_state)) {
		            printf("Obj%u: %8X\n", i, lastmsg.obj[i]);
			    if((i + 1) == (lastmsg.hdr >> 12)) {
				proc_state++;
			    }
			}
		    }
		    proc_state &= 0xF;
		    break;
		case (5) ://CRC
		    // Retrieve CRC (we don't currently do anything with it - TODO)
		    crc32_val = pd_bytes_to_reg(buf1, &buf1_output_count, &procbuf, &proc_freed_offset, &bmc_err_status, 4);
		    if(!pd_read_error_handler(&bmc_err_status, &proc_state)) {
		        proc_state++;
			if(crc32_pdmsg(lastmsg.bytes) == crc32_val) {
			    printf("CRC32 is Good! - %X\n", crc32_val);
			} else {
			    printf("CRC32 mismatch\n");
			}
		    }
		    break;
		case (6) ://EOP
		    // Attempt to retrieve EOP - TODO
		    if(pd_bytes_to_reg(buf1, &buf1_output_count, &procbuf, &proc_freed_offset, &bmc_err_status, 0) == 0xFF) {
			proc_state = 0;
			printf("EOP\n");
			//if(((lastmsg.hdr >> 12) & 0b111) && ((lastmsg.hdr & 0b11111) == 0b00001)) pd_respond_goodcrc(&lastmsg);
		    } else {
		    //TODO - add error handler function here (change proc_state in response to error)
		    }
		    break;
		default ://Error handler - TODO
		    break;
	}
*/
	/*
	if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
            printf("%16d - %08x\n", time_us_32(), pio_sm_get(pio, SM_RX));
	}
	*/
/*
    }
    */
}
