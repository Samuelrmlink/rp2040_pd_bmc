int bmcProcessSymbols(bmcDecode* bmc_d) {
    uint8_t input_offset = 0;
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
    while(bmc_d->pOffset >= 4 || !(bmc_d->pOffset)) {
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
		if(bmc_d->procBuf & 0b11111 != 0b01010) {//TODO - possibly change to identify start of ordered set
		    bmc_d->procSubStage = 0;	// Clear procSubStage data
		    bmc_d->procStage++;		// Increment procStage
		}
	        break;
	    case (1) :// Ordered set
		
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
    }
}
