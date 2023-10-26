#include "main_i.h"

int bmcProcessSymbols(bmcDecode* bmc_d, pd_msg* msg) {
    uint8_t input_offset = 0;
    bool breakout;
    // Ensure that no full symbols are present in the process buffer (design assumption)
    if(bmc_d->pOffset >= 4) {
	return -1; // Error - unprocessed symbols in 4b5b process buffer
    }

    // Copy some data from input buffer to process buffer
    uint8_t remainOffset = bmc_d->pOffset;
    bmc_d->procBuf |= bmc_d->inBuf << remainOffset;
    bmc_d->pOffset = 32; // No space left in process buffer (31 would mean 1 bit is free, 0 would mean 32 bits are free)
    	// It is expected that there may be remainder bits that won't fit into the procBuf

    // Run in a while loop until all full symbols have been processed
    while(bmc_d->pOffset >= 4) {
	// If there were remainder bits that can now fit into procBuf
	if(remainOffset && (bmc_d->pOffset <= 27)) {
	    bmc_d->procBuf |= (bmc_d->inBuf >> 32 - remainOffset) << bmc_d->pOffset;
	    bmc_d->pOffset += 32 - remainOffset;
	    remainOffset = 0;	// Reset offset
	} 
	
	// Switch depending on process stage
	switch(bmc_d->procStage & 0xF) {
	    case (0) :// Preamble
		// Shift out all bits except for one
		if(bmc_d->procBuf >> 8 == 0x555555) {
		    bmc_d->procBuf >>= 32 - 1;
		    bmc_d->pOffset = 1;
		}
		while((bmc_d->procBuf & 0b11) == 0b10) {
		    bmc_d->procBuf >> 2;
		    bmc_d->pOffset -= 2;
		}
//printf("DBG-REMOVE THIS: %X, %X\n", bmc_d->procBuf, bmc_d->pOffset);
		if(bmc_d->procBuf & 0b11111 == 0b00111 || bmc_d->procBuf & 0b11111 == 0b11000) { // If start of ordered set is found
		    bmc_d->procSubStage = 0;	// Clear procSubStage data
		    bmc_d->procStage++;		// Increment procStage
		}
breakout = true;//TODO-remove
	        break;
	    case (1) :// Ordered set
		uint internal_stage = 0;
		if(bmc_d->procSubStage & 0x0b11111000000000000000) internal_stage = 3;
		else if(bmc_d->procSubStage & 0x0b111110000000000) internal_stage = 2;
		else if(bmc_d->procSubStage & 0x0b1111100000) internal_stage = 1;
		// Otherwise internal_stage defaults to zero.

		// Store partial Ordered Set to procSubStage
		bmc_d->procSubStage <<= 5 * internal_stage;
	       	bmc_d->procSubStage |= bmc_d->procBuf & 0b11111;
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;

		if(internal_stage == 3) {
		    switch(bmc_d->procSubStage & 0xFFFFF) {
	    		case (0b11001001110011100111) : // Hard Reset
			    msg->_pad1[0] = 1;
			    break;
			case (0b00110001111100000111) : // Cable Reset
			    msg->_pad1[0] = 2;
			    break;
	    		case (0b10001110001100011000) : // SOP
			    msg->_pad1[0] = 3;
			    break;
	    		case (0b00110001101100011000) : // SOP'
			    msg->_pad1[0] = 4;
			    break;
	    		case (0b00110110000011011000) : // SOP''
			    msg->_pad1[0] = 5;
			    break;
	    		case (0b00110110011100111000) : // SOP' Debug
			    msg->_pad1[0] = 6;
			    break;
	    		case (0b10001001101100111000) : // SOP'' Debug
			    msg->_pad1[0] = 7;
			    break;
		    }
		}
		breakout = true;
		break;
	    case (2) :// PD Header
		break;
	    case (3) :// Extended Header (if applicable)
		break;
	    case (4) :// Data Objects (if applicable)
		break;
	    case (5) :// CRC32
		break;
	    case (6) :// EOP
		break;
	    default  ://Error
		//TODO - Implement error handling
	}
	if(breakout) break;
    }
}
