#include "main_i.h"
int8_t bmcDecode4b5b(uint8_t fiveB) {
   int8_t ret; 
    switch(fiveB & 0x1F) {
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
	default :	// Error condition - invalid symbol (possible K-code symbol or data corruption)
	    ret = 1 << 7 | fiveB;
	    break;
    }
    return ret;
}
void bmc_decode_clear(bmcDecode* bmc_d) {
    bmc_d->procStage = 0;
    bmc_d->procSubStage = 0;
    bmc_d->pioData.val = 0;
    bmc_d->procBuf = 0;
    bmc_d->pOffset = 0;
    bmc_d->crcTmp = 0;
}
void pd_frame_clear(pd_frame *pdf) {
    for(uint8_t i = 0; i < 56; i++) {
	pdf->raw_bytes[i] = 0;
    }
}
void pd_frame_queue_and_reset(bmcDecode* bmc_d, QueueHandle_t q_validPdf) {
    // Send complete pd_frame for evaluation (regardless of CRC validation status)
    xQueueSendToBack(q_validPdf, (void *) &bmc_d->msg, portMAX_DELAY);

    // Allocate another pd_frame
    bmc_d->msg = malloc(sizeof(pd_frame));

    // Reset process stage to zero
    bmc_d->procStage = 0;
    bmc_d->procSubStage = 0;
    bmc_d->crcTmp = 0;

    // Reset PD msg
    pd_frame_clear(bmc_d->msg);
}
int bmcProcessSymbols(bmcDecode* bmc_d, QueueHandle_t q_validPdf) {
    uint8_t internal_stage;
    uint8_t input_offset = 0;
    bool breakout = false;
    // Ensure that no full symbols are present in the process buffer (design assumption)
    if(bmc_d->pOffset > 4) {
	return -1; // Error - unprocessed symbols in 4b5b process buffer
    }

    // Copy some data from input buffer to process buffer
    uint8_t remainOffset = bmc_d->pOffset;
    bmc_d->procBuf |= bmc_d->pioData.val << remainOffset;
    bmc_d->pOffset = 32; // No space left in process buffer (31 would mean 1 bit is free, 0 would mean 32 bits are free)
    	// It is expected that there may be remainder bits that won't fit into the procBuf

    // Run in a while loop until all full symbols have been processed
    while(bmc_d->pOffset > 4 && !breakout) {

	// If there were remainder bits that can now fit into procBuf - TODO: move this into a function or macro
	if(remainOffset && (bmc_d->pOffset <= 27)) {
	    bmc_d->procBuf |= (bmc_d->pioData.val >> 32 - remainOffset) << bmc_d->pOffset;
	    bmc_d->pOffset += 32 - (32 - remainOffset);
	    remainOffset = 0;	// Reset offset
	} 
	
	// Switch depending on process stage
	switch(bmc_d->procStage & 0xF) {
	    case (0) :// Preamble
		while(((bmc_d->procBuf & 0b11) != 0b10) && ((bmc_d->procBuf & 0x1F) != 0x7) && ((bmc_d->procBuf & 0x1F) != 0x18) && (bmc_d->pOffset > 4)) {
		    bmc_d->procBuf >>= 1;
		    bmc_d->pOffset -= 1;
		}
		while((bmc_d->procBuf & 0b11) == 0b10) {
		    bmc_d->procBuf >>= 2;
		    bmc_d->pOffset -= 2;
		}
		// If there were remainder bits that can now fit into procBuf - TODO: move this into a function or macro
		if(remainOffset && (bmc_d->pOffset <= 27)) {
		    bmc_d->procBuf |= (bmc_d->pioData.val >> 32 - remainOffset) << bmc_d->pOffset;
		    bmc_d->pOffset += 32 - (32 - remainOffset);
		    remainOffset = 0;	// Reset offset
		}
		if(((bmc_d->procBuf & 0x3FF) == 0b0011100111) || ((bmc_d->procBuf & 0x3FF) == 0b1100000111) || ((bmc_d->procBuf & 0x1F) == 0x18)) {
		    bmc_d->procSubStage = 0;
		    bmc_d->procStage++;
		}
	        break;
	    case (1) :// Ordered set
		internal_stage = 0;
		if(bmc_d->procSubStage & 0b111110000000000) internal_stage = 3;
		else if(bmc_d->procSubStage & 0b1111100000) internal_stage = 2;
		else if(bmc_d->procSubStage & 0b11111) internal_stage = 1;
		// Otherwise internal_stage defaults to zero.

		// Store partial Ordered Set to procSubStage
		bmc_d->procSubStage |= (bmc_d->procBuf & 0x1F) << 5 * internal_stage;
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;

		if(internal_stage == 3) {
		    switch(bmc_d->procSubStage & 0xFFFFF) {
	    		case (0b11001001110011100111) : // Hard Reset
			    bmc_d->msg->frametype = 1;
			    bmc_d->procStage = 0;
			    break;
			case (0b00110001111100000111) : // Cable Reset
			    bmc_d->msg->frametype = 2;
			    bmc_d->procStage = 0;
			    break;
	    		case (0b10001110001100011000) : // SOP
			    bmc_d->msg->frametype = 3;
			    break;
	    		case (0b00110001101100011000) : // SOP'
			    bmc_d->msg->frametype = 4;
			    break;
	    		case (0b00110110000011011000) : // SOP''
			    bmc_d->msg->frametype = 5;
			    break;
	    		case (0b00110110011100111000) : // SOP' Debug
			    bmc_d->msg->frametype = 6;
			    break;
	    		case (0b10001001101100111000) : // SOP'' Debug
			    bmc_d->msg->frametype = 7;
			    break;
			default:
			    break;
		    }
		    if(bmc_d->procStage) {
			bmc_d->procStage++;
			bmc_d->procSubStage = 0;
		    } else {
			pd_frame_queue_and_reset(bmc_d, q_validPdf);
		    }
		}
		break;
	    case (2) :// PD Header
		// Process one symbol
		bmc_d->msg->hdr |= bmcDecode4b5b(bmc_d->procBuf & 0x1F) << (4 * bmc_d->procSubStage);
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;
		if(bmc_d->procSubStage == 3) { // If full header has been received
		    if((bmc_d->msg->hdr >> 15) & 0x1) {
			bmc_d->procStage++; // Extended header follows
			bmc_d->procSubStage = 0;
		    } else if((bmc_d->msg->hdr >> 12) & 0x7) {
			bmc_d->procStage += 2; // Data objects follow
			bmc_d->procSubStage = 0;
		    } else {
			bmc_d->procStage += 3; // No data objects - control message
			bmc_d->procSubStage = 0;
		    }
		} else bmc_d->procSubStage++;  // Increment & wait for next symbol in PD header
		break;
	    case (3) :// Extended Header (if applicable)
		bmc_d->msg->obj[0] |= bmcDecode4b5b(bmc_d->procBuf & 0x1F) << 4 * (bmc_d->procSubStage % 8);
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;
		bmc_d->procSubStage++;
		// Once Extended Header has been received
		if(bmc_d->procSubStage == 4) {
		    if(bmc_d->msg->obj[0] >> 15) { // Chunked bit is set
			// Continue process in the next stage
			bmc_d->procStage++;
			// Don't change the procSubStage to ensure continution of the first
			// data object. (With extended header as part of that first object.)
		    } else {
			// TODO - implement unchunked extended messages
		    }
		}
		break;
	    case (4) :// Data Objects (if applicable)
		bmc_d->msg->obj[bmc_d->procSubStage / 8] |= bmcDecode4b5b(bmc_d->procBuf & 0x1F) << 4 * (bmc_d->procSubStage % 8);
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;
		bmc_d->procSubStage++;

		// Check whether this is the last half-byte in the last data object
		if(bmc_d->procSubStage == 8 * ((bmc_d->msg->hdr >> 12) & 0x7)) {
		    bmc_d->procStage++;
		    bmc_d->procSubStage = 0;
		}
		break;
	    case (5) :// CRC32
		bmc_d->crcTmp |= bmcDecode4b5b(bmc_d->procBuf & 0x1F) << (4 * bmc_d->procSubStage);
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;
		bmc_d->procSubStage++;

		if(bmc_d->procSubStage == 8) { // Move to next stage
		    bmc_d->procStage++;
		    bmc_d->procSubStage = 0;
		}
		break;
	    case (6) :// EOP
		// If EOP is received && CRC is valid
		if(((bmc_d->procBuf & 0x1F) == 0b01101) && (crc32_pdframe_calc(bmc_d->msg) == bmc_d->crcTmp)) {
		    bmc_d->msg->frametype |= 1 << 7;
		} else {
		}
		bmc_d->procBuf >>= 5;
		bmc_d->pOffset -= 5;

		// Add to valid frame queue (regardless of CRC status) && reset the decode variables
		pd_frame_queue_and_reset(bmc_d, q_validPdf);
		break;
	    default  ://Error
		//TODO - Implement error handling
		break;
	}
	// Assign starting timestamp to new PD frame (ONLY AFTER exiting the preamble stage!)
	if(!bmc_d->msg->timestamp_us && bmc_d->procStage) {
	    bmc_d->msg->timestamp_us = bmc_d->pioData.time;
	}
    }
}
