#include "main_i.h"

void pd_frame_clear(pd_frame *pdf) {
    for(uint8_t i = 0; i < 56; i++) {
	pdf->raw_bytes[i] = 0;
    }
}
void pdf_generate_goodcrc(pd_frame *input_frame, pd_frame *output_frame) {
    // Ensure we start with a clean slate
    pd_frame_clear(output_frame);

    // Transfer over frame type (SOP, SOP', SOP", etc..)
    output_frame->ordered_set = input_frame->ordered_set;

    // Transfer the MsgID, Spec Rev. and apply the GoodCRC Msg Type.
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    output_frame->hdr = (input_frame->hdr & 0xE00) | (input_frame->hdr & 0xC0) | (0x2 << 6) | (uint8_t)controlMsgGoodCrc;

    // Generate CRC32
    output_frame->obj[0] = crc32_pdframe_calc(output_frame);
}
PDMessageType pdf_get_sop_msg_type(pd_frame *msg) {
    uint8_t frmType = 0;
    if(msg->hdr & 0x8000) {		// Extended message
	frmType = 1 << 7;
    } else if(msg->hdr & 0x7000) {	// Data message
	frmType = 1 << 6;
    } else {				// Control messsage
	frmType = 0;
    }
    frmType |= msg->hdr & 0x1f;
    return (PDMessageType) frmType;
}
bool is_src_cap(pd_frame *pdf) {
    if((bmc_get_ordset_index(pdf->ordered_set) == PdfTypeSop) && (pdf_get_sop_msg_type(pdf) == dataMsgSourceCap)) {
        return true;
    } else {
        return false;
    }
}
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
uint32_t *obj2;
void thread_rx_process(void* unused_arg) {
    extern bmcChannel *bmc_ch0;
    extern bmcRx *pdq_rx;
    extern bmcTx *tx;

    // Clear all memory allocated to incoming pd_frame data
    for(int i = 0; i < pdq_rx->rolloverObj; i++) {
        pd_frame_clear(&(pdq_rx->pdfPtr)[i]);
    }

    // Setup toggle pin (used for debugging)
    gpio_init(16);
    gpio_set_dir(16, GPIO_OUT);

	uint8_t proc_counter = 0;
    uint8_t tmpindex;
    pdo_accept_criteria power_req = {
        .mV_min = 5000,
        .mV_max = 10240,
        .mA_min = 0,
        .mA_max = 2000
    };
    pd_frame *cPdf;
    tx = malloc(sizeof(bmcTx));
    tx->pdf = malloc(sizeof(pd_frame));
    pd_frame_clear(tx->pdf);

    while(true) {
    // If proc_counter is ready to rollover
    if(proc_counter == pdq_rx->rolloverObj) {
        // Clear the inputRollover variable
        pdq_rx->inputRollover = false;
        // Reset proc_counter
        proc_counter = 0;
        // Clear pd_frame arrays
        for(int i = pdq_rx->objOffset; i < pdq_rx->rolloverObj; i++) {
            pd_frame_clear(&(pdq_rx->pdfPtr)[i]);
        }
    }
	// If there is a complete frame
	if((pdq_rx->objOffset > proc_counter) || pdq_rx->inputRollover) {
	    individual_pin_toggle(16);
	    // Current pd_frame pointer (added for improved readability)
	    cPdf = &(pdq_rx->pdfPtr)[proc_counter];
	    proc_counter++;
	    if(bmc_validate_pdf(cPdf) && !cPdf->__padding1) {
		if(bmc_get_ordset_index(cPdf->ordered_set) == PdfTypeSop) {
		    pdf_generate_goodcrc(cPdf, tx->pdf);
		    pdf_transmit(tx, bmc_ch0);
		    if(is_src_cap(cPdf)) {
			tmpindex = optimal_pdo(cPdf, power_req);
			if(!tmpindex) { tmpindex = 1; }   // If no acceptable PDO is found - just request the first one (always 5v)
			pdf_request_from_srccap(cPdf, tx, tmpindex, power_req);
			pdf_transmit(tx, bmc_ch0);
		    }
		}
		//printf("%s %X\n", sopFrameTypeNames[bmc_get_ordset_index(cPdf->ordered_set)], cPdf->hdr);
		cPdf->__padding1 = 1;
	    }
	    if(pdq_rx->inputRollover) {
		    pdq_rx->inputRollover = false;
	    }
	} else {
        if(bmc_get_timestamp(pdq_rx) && !bmc_rx_active(bmc_ch0)) {
            // Fill the RX FIFO with zeros until it pushes
        //    individual_pin_toggle(16);
            while(pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
                pio_sm_exec_wait_blocking(bmc_ch0->pio, bmc_ch0->sm_rx, pio_encode_in(pio_y, 1));
            }
            // Retrieve data from the RX FIFO
            bmc_rx_cb();
        //    individual_pin_toggle(16);
        }
    if(pio_sm_get_pc(bmc_ch0->pio, bmc_ch0->sm_tx) == 27) { // THIS IS A HACK - 27 is the PIO instruction that (at the time of this hack) leaves the tx line pulled high
      pio_sm_exec(bmc_ch0->pio, bmc_ch0->sm_tx, pio_encode_jmp(22) | pio_encode_sideset(1, 1));
    }
	}
    sleep_us(100);
    }
}
