void bmcProcessSymbols(bmcDecode* bmc_d) {
    while(bmc_d->pOffset < 4 && !(bmc_d->pOffset)) {
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
