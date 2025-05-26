#include "main_i.h"

void pd_frame_clear(pd_frame *pdf) {
    for(uint8_t i = 0; i < 56; i++) {
	pdf->raw_bytes[i] = 0;
    }
}
void pdf_debug_printframe(pd_frame *pdf) {
    printf("# Hdr: %03X | ", pdf->hdr);
    for(int i = 0; i < 56; i++) {
        printf("%02X", pdf->raw_bytes[i]);
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
uint8_t pdf_get_msgid(pd_frame *msg) {
    return (msg->hdr >> 9) & 0x7;
}
// Check MsgType (For example: controlMsgGoodCrc)
bool is_sop_msgtype(pd_frame *pdf, uint8_t msgtype) {
    bool ret = (pdf_get_sop_msg_type(pdf) == controlMsgGoodCrc) ? true : false;
    return ret;
}
bool is_src_cap(pd_frame *pdf) {
    if((bmc_get_ordset_index(pdf->ordered_set) == PdfTypeSop) && is_sop_msgtype(pdf, dataMsgSourceCap)) {
        return true;
    } else {
        return false;
    }
}
// Check whether this msgID has been processed or not
bool check_msgid_done(int8_t global_msgid, int8_t current_msgid) {
    if(global_msgid == -1) {
        // msgID was just initialized - so this means that no messages have been processed yet.
        return false;
     } else if(global_msgid == current_msgid) {
        // msgID marked as processed (Probably retransmitted by our port partner (SOP) or cable (SOP',SOP", etc.) due to timeout)
        return true;
    } else {
        // For now - any other case means that ***MOST LIKELY*** this is the next frame that hasn't been evaluated yet
        return false;
    }
}
// Mark this msgID as processed
void mark_msgid_done(int8_t *global_msgid, int8_t current_msgid) {
    // Get next 'would-be' global_msgid (assuming we increment by 1)
    int8_t tmp_val = (*global_msgid == 7) ? 0 : *global_msgid + 1;
    if(tmp_val != current_msgid) {
        // TODO: Warn user that we may be missing some PD frames (maybe in a more 'proper' fashion than below...)
        printf("mark_msgid_done() - We may be missing some PD frames!");
    }
    // Mark as processed
    *global_msgid = current_msgid;
}
// Check whether TCPC thread has evaluated this pd_frame
bool check_pdf_tcpc_eval(pd_frame *pdf) {
    // __padding1 - Bit0 indicates that the port controller thread has accepted & evaluated this frame
    return (bool) pdf->__padding1 & 0x1;
}
// Mark this pd_frame as evaluated (by the USB Type-C port controller thread)
// Note: This indicates that the frame is valid & was accepted by the TCPC thread - but does
// *NOT* necessarily indicate that it was passed to the TCPM thread or made it to the Policy Engine.
void mark_pdf_tcpc_eval(pd_frame *pdf) {
    pdf->__padding1 |= 0x1; // Set Bit0
}
// Check __padding1 flag (Single flag only)
bool check_pdf_flag(pd_frame *pdf, uint8_t flag) {
    return (pdf->__padding1) & (0x1 << flag);
}
// Mask (set) __padding1 flag
void mark_pdf_flag(pd_frame *pdf, uint8_t flag) {
    pdf->__padding1 |= (0x1 << flag);
}

/*
// Setup toggle pin (used for debugging)
gpio_init(16);
gpio_set_dir(16, GPIO_OUT);
gpio_init(17);
gpio_set_dir(17, GPIO_OUT);
individual_pin_toggle(16);
*/


// Polling func: Maintains procbuf ringbuffer (handles rollover)
void poll_manage_procbuf(uint8_t proc_count) {
    extern bmcRx *pdq_rx;
    // If proc_counter is ready to rollover
    if(proc_count == pdq_rx->rolloverObj) {
        // Clear the inputRollover variable
        pdq_rx->inputRollover = false;
        // Reset proc_counter
        proc_count = 0;
        // Clear pd_frame arrays
        for(int i = pdq_rx->objOffset; i < pdq_rx->rolloverObj; i++) {
            pd_frame_clear(&(pdq_rx->pdfPtr)[i]);
        }
    }
}
// Polling func: Returns true when new pd_frame data is available
// Returns: [-1]Corrupted new data, [0]No new data, [1]Valid new data
int8_t new_data_procbuf(uint8_t proc_count) {
    extern bmcRx *pdq_rx;
    pd_frame *cPdf = &(pdq_rx->pdfPtr)[proc_count];
    if(!((pdq_rx->objOffset > proc_count) || pdq_rx->inputRollover)) {
        // No new data
        return 0;
    }
    if(bmc_validate_pdf(cPdf)               // Validate CRC
       && !check_pdf_tcpc_eval(cPdf)) {     // Validate that TCPC has not handled this frame before
       // TODO: Implement check_msgid_done() here

        // Valid new data
        return 1;
    }
    // Corrupted new data
    return -1;
}
// Polling func: Clears PIO interfaces as required
void manage_pio_rxbuf() {
    extern bmcRx *pdq_rx;
    extern bmcChannels *bmc_ch;
    bmcChannel *bmc_ch0 = &(bmc_ch->chan)[0];
    if(bmc_get_timestamp(pdq_rx) && !bmc_rx_active(bmc_ch0)) {
        // Fill the RX FIFO with zeros until it pushes
        while(pio_sm_is_rx_fifo_empty(bmc_ch0->pio, bmc_ch0->sm_rx)) {
            pio_sm_exec_wait_blocking(bmc_ch0->pio, bmc_ch0->sm_rx, pio_encode_in(pio_y, 1));
        }
        // Retrieve data from the RX FIFO
        bmc_rx_cb();
    }
    // THIS IS A HACK - instruction 27 is the PIO instruction that (at the time of this hack) leaves the tx line pulled high
    if(pio_sm_get_pc(bmc_ch0->pio, bmc_ch0->sm_tx) == 27) {
        pio_sm_exec(bmc_ch0->pio, bmc_ch0->sm_tx, pio_encode_jmp(22) | pio_encode_sideset(1, 1));
    }
}


// Global variables
uint32_t *obj2;
uint8_t srccap_index;
//extern uint32_t config_reg[CONFIG_NUMBER_OF_REGISTERS];
uint32_t config_reg[CONFIG_NUMBER_OF_REGISTERS] = {0xFA042, 0x17D000, 0x98}; //TEST - TODO: remove
char* string_ptr[CONFIG_NUMBER_OF_STRINGS] = {NULL};
extern configKey* config_db = database;


// USB-PD Port Controller thread 
void thread_pd_portctrl(void* unused_arg) {
    // USB-PD PHY RX/TX channel structures
    extern bmcRx *pdq_rx;   // RX queue
    extern bmcTx *tx;       // TX queue
    extern bmcChannels *bmc_ch;                 // Channels list
    bmcChannel *bmc_ch0 = &(bmc_ch->chan)[0];   // Channel 1 pointer

    // USB-PD 
    extern QueueHandle_t queue_pc_in;
    extern QueueHandle_t queue_pe_in;

    pd_frame *cPdf;         // Current *pd_frame
    uint8_t proc_counter = 0;
    int8_t sop_rx_msgid = -1;

    // Debug stuff
    gpio_init(2);
    gpio_set_dir(2, GPIO_OUT);


    while(true) {
        poll_manage_procbuf(proc_counter);      // Data buffer maintenance (rollover/data overwrite)
        
        // Grab pd_frame pointer (convenience)
        cPdf = &(pdq_rx->pdfPtr)[proc_counter];
        switch(new_data_procbuf(proc_counter)) {
            case(-1):   // Corrupt new data
                // Mark as corrupt
                mark_pdf_flag(cPdf, tcpcFrameInvalid);
                // Skip over data
                proc_counter++;
                break;
            case(0):    // No new data
                manage_pio_rxbuf(); // Clears PIO interfaces as required
                // Do nothing.. Will wait until more data arrives
                break;
            case(1):    // Valid new data
                // Check frame type (SOP, SOP', SOP", etc.) and compare against TCPC filtering policy
                switch(bmc_get_ordset_index(cPdf->ordered_set)) {
                    case(PdfTypeHardReset):
                        // SOP' type
                        // TODO: Implement debug & logging to non-vol flash
                        printf("Hard Reset\n");
                        break;
                    case(PdfTypeCableReset):
                        // SOP' type
                        // TODO: Implement debug & logging to non-vol flash
                        printf("Cable Reset\n");
                        break;
                    case(PdfTypeSop):
                        // SOP type
                        if(!is_sop_msgtype(cPdf, controlMsgGoodCrc)) {
                            pdf_generate_goodcrc(cPdf, tx->pdf);
                            pdf_transmit(tx, bmc_ch0);
                            printf("SOP\n");
                            // TODO: Implement pd_frame filtering system
                            //if(mcu_reg_get_uint(&key_sop_accept, false)) {
                            xQueueSendToBack(queue_pe_in, (void *) cPdf, (TickType_t) 0);
                        } else {
                            // Debug - TODO: remove
                            printf(" CRC ");
                        }
                        break;
                    case(PdfTypeSopP):
                        // SOP' type
                        // TODO: Implement
                        //printf("SOPP\n");
                        break;
                    case(PdfTypeSopDp):
                        // SOP" type
                        // TODO: Implement
                        //printf("SOPDp\n");
                        break;
                    case(PdfTypeSopPDbg):
                        // SOP' Debug type
                        // TODO: Implement
                        //printf("SOPPDbg\n");
                        break;
                    case(PdfTypeSopDpDbg):
                        // SOP" Debug type
                        // TODO: Implement
                        //printf("SOPDpDbg\n");
                        break;
                    default:
                        // We should never reach here
                        printf("Error: Unknown pd_frame type: [0x%X]", cPdf->ordered_set);
                }
                // Mark as processed by TCPC
                mark_pdf_tcpc_eval(cPdf);
                // Increment proc_counter
                proc_counter++;
                break;
        }
        /*
        if(new_data_procbuf(proc_counter)) {
            // Fetch the fresh pd_frame
            cPdf = &(pdq_rx->pdfPtr)[proc_counter];
            // Increment the processing buffer
            proc_counter++;

            // Check whether this pd_frame is valid
            if(bmc_validate_pdf(cPdf) && !check_pdf_tcpc_eval(cPdf)) {
                if(bmc_get_ordset_index(cPdf->ordered_set) == PdfTypeSop && pdf_get_sop_msg_type(cPdf) != controlMsgGoodCrc) {
                    pdf_generate_goodcrc(cPdf, tx->pdf);
                    pdf_transmit(tx, bmc_ch0);
                    //printf("GCRC %X", cPdf->hdr);
                    pdf_debug_printframe(cPdf);
                    if(mcu_reg_get_uint(&key_sop_accept, false)) {
                        // TODO: Share the PDO with the PE : cPdf
                        xQueueSendToBack(queue_pe_in, (void *) cPdf, (TickType_t) 0);
                        //printf("T");
                        individual_pin_toggle(2);
                        //printf("Share SRCCAP\n");
                    }
                }
                mark_pdf_tcpc_eval(cPdf);
            }
            if(pdq_rx->inputRollover) {
                pdq_rx->inputRollover = false;
            }
        } else { // No new data is available
            manage_pio_rxbuf(); // Clears PIO interfaces as required
        }
        */
    sleep_us(100);
    }
}
