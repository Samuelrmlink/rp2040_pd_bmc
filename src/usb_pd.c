#include "main_i.h"

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
bool is_crc_good(pd_frame *pdf) {
    if(pdf->frametype & 0x80)
	return true;
    else
	return false;
}
bool is_sop_frame(pd_frame *pdf) {
    if((pdf->frametype & 0x7) == 3)
	return true;
    else 
	return false;
}
PDMessageType pdf_get_sop_msg_type(pd_frame *msg) {
    uint8_t msgType = 0;
    if(msg->hdr & 0x8000) {		// Extended message
	msgType = 1 << 7;
    } else if(msg->hdr & 0x7000) {	// Data message
	msgType = 1 << 6;
    } else {				// Control messsage
	msgType = 0;
    }
    msgType |= msg->hdr & 0x1f;
    return (PDMessageType) msgType;
}
bool is_src_cap(pd_frame *pdf) {
    if(((pdf->frametype & 0x7) == PdfTypeSop) && (pdf_get_sop_msg_type(pdf) == dataMsgSourceCap)) {
        return true;
    } else {
        return false;
    }
}
bool eval_pdo_fixed(uint32_t pdo_obj, pdo_accept_criteria req) {
    // We know this is a fixed PDO - not Augmented, Battery, etc..
    uint32_t pdo_mV = ((pdo_obj >> 10) & 0x3FF) * 50;
    if(pdo_mV <= req.mV_max && pdo_mV >= req.mV_min) {
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
 */
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
	    pMsg.msgType = peMsgPdFrame;
	    pMsg.pdf = rxdPdf;
	    xQueueSendToBack(queue_policy, &pMsg, portMAX_DELAY);
	    /*
	    // Free memory at pointer to avoid memory leak
	    free(rxdPdf);
	    */
	}
    }
}
