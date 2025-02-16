#include "main_i.h"
#include "mcu_registers.h"

/*
uint8_t proc_counter = 0;
    pdo_accept_criteria power_req = {
        .mV_min = 5000,
        .mV_max = 10240,
        .mA_min = 0,
        .mA_max = 2000
    };
*/
/*  -- Probably don't need this?-???
    // Clear all memory allocated to incoming pd_frame data
    for(int i = 0; i < pdq_rx->rolloverObj; i++) {
        pd_frame_clear(&(pdq_rx->pdfPtr)[i]);
    }
*/




bool eval_pdo_fixed(uint32_t pdo_obj, pdo_accept_criteria req) {
    // We know this is a fixed PDO - not Augmented, Battery, etc..
    uint32_t pdo_mV = ((pdo_obj >> 10) & 0x3FF) * 50;
    uint16_t pdo_mA_max = (pdo_obj & 0x3FF) * 10;
    if(pdo_mV <= req.mV_max && pdo_mV >= req.mV_min && pdo_mA_max >= req.mA_min) {
	return true;
    } else {
	return false;
    }
}
bool eval_pdo_augmented(uint32_t pdo_obj, pdo_accept_criteria req) {
    if(!((pdo_obj >> 28) & 0x3)) {		// SPR PPS
	// Get PPS PDO values
	uint32_t pdo_mV_max = ((pdo_obj >> 17) & 0xFF) * 100;
	uint32_t pdo_mV_min = ((pdo_obj >> 8) & 0xFF) * 100;
	uint16_t pdo_mA_max = (pdo_obj & 0x7F) * 50;

	// Compare PDO values with the criteria
	if(pdo_mV_max >= req.mV_max && pdo_mV_max >= req.mV_min && pdo_mA_max >= req.mA_min) {
	    return true;
	}
    } else if(((pdo_obj >> 28) & 0x3) == 0x2) {	// EPR
	// TODO: Implement
	return false;
    } else {
	// Invalid
	return false;
    }
}
bool pdo_is_augmented(pd_frame *srccap_pdf, uint8_t index) {
    uint32_t pdo_obj = srccap_pdf->obj[index];
    if(((pdo_obj >> 30) & 0x3 == pdoTypeAugmented) && !((pdo_obj >> 28) & 0x3)) {
        return true;
    } else {
        return false;
    }
}
uint8_t optimal_pdo(pd_frame *pdf, pdo_accept_criteria power_req) {
    uint8_t ret = 0;
    // We are already assuming for this function that a valid Source Cap message is being passed in
    for(int i = 1; i <= ((pdf->hdr >> 12) & 0x7); i++) {
	switch((pdf->obj[i - 1] >> 30) & 0x3) {
	    case (pdoTypeFixed) :
		if(eval_pdo_fixed(pdf->obj[i - 1], power_req)) {
		    ret = i;
		}
		break;
	    case (pdoTypeAugmented) :
		if(eval_pdo_augmented(pdf->obj[i - 1], power_req)) {
		    ret = i;
		}
		break;
	    case (pdoTypeBattery) :
	    case (pdoTypeVariable) :
		break;
	    default :
		//TODO - Implement error handling
		break;
	}
    }
    return ret;
}

// TODO - move into a "Policy Engine"
void pdf_request_from_srccap(pd_frame *input_frame, bmcTx *tx, uint8_t req_pdo, pdo_accept_criteria req) {
    switch((input_frame->obj[req_pdo - 1] >> 30) & 0x3) {
        case (pdoTypeFixed) :
            pdf_request_from_srccap_fixed(input_frame, tx, req_pdo, req);
            break;
        case (pdoTypeAugmented) :
            pdf_request_from_srccap_augmented(input_frame, tx, req_pdo, req);
            break;
        case (pdoTypeBattery) :
        case (pdoTypeVariable) :
            break;
        default :
            //TODO - Implement error handling
            break;
    }
}
void pdf_request_from_srccap_fixed(pd_frame *input_frame, bmcTx *tx, uint8_t req_pdo, pdo_accept_criteria req) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Get PDO maximum current
    uint16_t mA_max = (input_frame->obj[req_pdo - 1] & 0x3FF) * 10;

    // Replace maximum current value if requested value is lower
    if(req.mA_max < mA_max) {
        mA_max = req.mA_max;
    }


    // Setup ordered_set (SOP) and header (uses hard-coded values for testing currently)
    tx->pdf->ordered_set = input_frame->ordered_set;
    tx->msgIdOut = 0;//TODO-fix this mess
    tx->pdf->hdr = (0x1 << 12) | (tx->msgIdOut << 9) | (0x2 << 6) | 0x2; // TODO - remove magic numbers

    // Setup RDO
    tx->pdf->obj[0] =	(req_pdo << 28) |			// Object position
    (((mA_max / 10) & 0x3FF) << 10) |	// Operating current
    ((mA_max / 10) & 0x3FF);				// Max current

    // Generate CRC32
    tx->pdf->obj[1] = crc32_pdframe_calc(tx->pdf);
}
void pdf_request_from_srccap_augmented(pd_frame *input_frame, bmcTx *tx, uint8_t req_pdo, pdo_accept_criteria req) {
    // Ensure we start with a clean slate
    pd_frame_clear(tx->pdf);

    // Since eval_pdo_augmented has passed we know the req_pdo.mV_max is within range of this VPDO
    // Get PDO max current
    uint32_t pdo_mA_max = (input_frame->obj[req_pdo - 1] & 0x7F) * 50;

    // Any modification of the current value does not leave this function
    if(req.mA_max > pdo_mA_max) {
        // Current requested is higher than provided by the charger - just take what we can get
        req.mA_max = pdo_mA_max;
    }

    // Setup ordered_set (SOP) and header (uses hard-coded values for testing currently)
    tx->pdf->ordered_set = input_frame->ordered_set;
    tx->msgIdOut = 0;//TODO-fix this mess
    tx->pdf->hdr = (0x1 << 12) | (tx->msgIdOut << 9) | (0x2 << 6) | 0x2; // TODO - remove magic numbers

    // Setup RDO
    tx->pdf->obj[0] =	(req_pdo << 28) |			// Object position
    (((req.mV_max / 20) & 0xFFF) << 9) |	// Output voltage
    ((req.mA_max / 50) & 0x7F);		// Operating current

    // Generate CRC32
    tx->pdf->obj[1] = crc32_pdframe_calc(tx->pdf);
}