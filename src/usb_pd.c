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
/*
bool is_crc_good(pd_frame *pdf) {
    return (bool)(pdf->frametype & 0x80);
}
bool check_sop_type(uint8_t type, pd_frame *pdf) {
    return (bool)((pdf->frametype & 0x7) == type);
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
    if(((pdf->frametype & 0x7) == PdfTypeSop) && (pdf_get_sop_msg_type(pdf) == dataMsgSourceCap)) {
        return true;
    } else {
        return false;
    }
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
        /*
	if(((pdf->obj[i - 1] >> 10) & 0x3FF) * 50 == req_mvolts) {
            ret = i;
        }
	*/
    }
    return ret;
}
/*
void pdf_generate_request(pd_frame *pdf, txFrame *txf, uint8_t req_index) {
  // Ensure we start with a clean slate
  pd_frame_clear(tx->pdf);

  // Setup frametype (SOP) and header (uses hard-coded values for testing currently)
  tx->pdf->frametype = PdfTypeSop;
  tx->pdf->hdr = (0x1 << 12) | (tx->msgIdOut << 9) | (0x2 << 6) | 0x2; // TODO - remove magic numbers

  // Setup RDO
  tx->pdf->obj[0] =	(0x1 << 28) |			// Object position
			((input_frame->obj[0] & 0x3FF) << 10) |	// Operating current
			(input_frame->obj[0] & 0x3FF);		// Max current
  
  // Generate CRC32
  tx->crc = crc32_pdframe_calc(tx->pdf);
}*/


/*
 *	USB-PD PIO data -> pd_frame data structure (as defined in pdb_msg.h header file)
 *
void thread_rx_process(void* unused_arg) {
    extern QueueHandle_t queue_rx_pio;
    extern QueueHandle_t queue_rx_validFrame;
    extern QueueHandle_t queue_policy;
    uint32_t rxval;
    bmcDecode *bmc_d = malloc(sizeof(bmcDecode));
    bmc_d->msg = malloc(sizeof(pd_frame));
    pd_frame *rxdPdf = NULL;
    policyEngineMsg pMsg;

    // Clear variables
    bmc_decode_clear(bmc_d);
    pd_frame_clear(bmc_d->msg);

    while(true) {
	// Block this thread if we are in the awaiting preamble stage
	if(!bmc_d->procStage) {
	    // Await new data from the BMC PIO ISR (blocking function)
	    xQueueReceive(queue_rx_pio, &bmc_d->pioData, portMAX_DELAY);
	// Otherwise check the queue without blocking (provide some time)
	} else {
	    if(!xQueueReceive(queue_rx_pio, &bmc_d->pioData, 0)) {
		sleep_us(120);
		if(!xQueueReceive(queue_rx_pio, &bmc_d->pioData, 0)) {
		    // TODO - add PIO flush function here
		    irq_set_enabled(bmc_ch0->irq, false);
		    while(pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
			pio_sm_exec_wait_blocking(bmc_ch0->pio, bmc_ch0->sm_rx, pio_encode_in(pio_y, 1));
		    }
		    irq_set_enabled(bmc_ch0->irq, true);
		    bmc_rx_check();
		    continue;
		}
	    }
	}
        bmcProcessSymbols(bmc_d, queue_rx_validFrame);
        
	// Check for complete pd_frame data
	if(xQueueReceive(queue_rx_validFrame, &rxdPdf, 0) && is_crc_good(rxdPdf)) { // If rxd && CRC is valid
	    pMsg.msgType = peMsgPdFrameIn;
	    pMsg.pdf = rxdPdf;
	    xQueueSendToBack(queue_policy, &pMsg, portMAX_DELAY);
	}
    }
}
*/
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
    pd_frame *cPdf;
    tx = malloc(sizeof(bmcTx));
    tx->pdf = malloc(sizeof(pd_frame));
    pd_frame_clear(tx->pdf);

    while(true) {
	// If there is a complete frame
    if(pdq_rx->objOffset > proc_counter) {
        // Current pd_frame pointer (added for improved readability)
        cPdf = &(pdq_rx->pdfPtr)[proc_counter];
        proc_counter++;
        if(crc32_pdframe_valid(cPdf) && !cPdf->__padding1) {
            if(bmc_get_ordset_index(cPdf->ordered_set) == PdfTypeSop) {
                pdf_generate_goodcrc(cPdf, tx->pdf);
                pdf_transmit(tx, bmc_ch0);
            }
            //printf("%s %X\n", sopFrameTypeNames[bmc_get_ordset_index(cPdf->ordered_set)], cPdf->hdr);
            cPdf->__padding1 = 1;
        }
		if(pdq_rx->inputRollover && (proc_counter == 255)) {
			pdq_rx->inputRollover = false;
		}
	} else {
        if(bmc_get_timestamp(pdq_rx) && !bmc_rx_active(bmc_ch0)) {
            // Fill the RX FIFO with zeros until it pushes
            individual_pin_toggle(16);
            while(pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
                pio_sm_exec_wait_blocking(bmc_ch0->pio, bmc_ch0->sm_rx, pio_encode_in(pio_y, 1));
            }
            // Retrieve data from the RX FIFO
            bmc_rx_cb();
            individual_pin_toggle(16);
        }
    }
    sleep_us(100);
    }
}
