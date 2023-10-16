int bmcProcessSymbols(bmcDecode* bmc_d) {
    uint8_t input_offset = 0;
    // Ensure that no full symbols are present in the process buffer (design assumption)
    if(bmc_d->pOffset >= 4) {
	return -1; // Error - unprocessed symbols in 4b5b process buffer
    }

    // Run in a while loop until all full symbols have been processed
    while(bmc_d->pOffset >= 4 || !(bmc_d->pOffset)) {
	if(bmc_d->pOffset < 32 && input_offset != 32) {
	    //TODO - pickup here
	}
    	// Switch depending on process stage
    	switch(procStage & 0xF) {
	    case (0) :// Preamble
		//Some task
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
