#include "main_i.h"

bool pe_eval_pdo_fixed(uint32_t pdo_obj, peSinkPowerCriteria req) {
    // We know that this is a fixed PDO - not Augmented, Battery, etc..
    uint pdo_mV = ((pdo_obj >> 10) & 0x3FF) * 50;
    uint pdo_mA_max = (pdo_obj & 0x3FF) * 10;
    return (pdo_mV <= req.mV_max && pdo_mV >= req.mV_min && pdo_mA_max >= req.mA_min) ? true : false;
}
bool pe_eval_pdo_augmented(uint32_t pdo_obj, peSinkPowerCriteria req) {
    switch((pdo_obj >> 28) & 0x3) {
        case(pdoTypeAugmentedSprPps):
            // Get SPR PPS PDO values
            uint32_t pdo_mV_max = ((pdo_obj >> 17) & 0xFF) * 100;
            uint32_t pdo_mV_min = ((pdo_obj >> 8) & 0xFF) * 100;
            uint pdo_mA_max = (pdo_obj & 0x7F) * 50;
            // Compare PDO values with the criteria
            if(pdo_mV_max >= req.mV_min && pdo_mA_max >= req.mA_min) { return true; }
            break;
        case(pdoTypeAugmentedEprAvs):
            //cli_log(DEBUG_LOG, "EPR AVS\n");
            break;
        case(pdoTypeAugmentedSprAvs):
            //cli_log(DEBUG_LOG, "SPR AVS\n");
            break;
        default:
            //cli_log(DEBUG_LOG, "Invalid/Reserved\n");
    }
    return false;
}
uint optimal_pdo(pd_frame *pdf, peSinkPowerCriteria power_req) {
    uint ret = 0;
    // We are already assuming for this function that a valid Source Cap message is being passed in
    for(uint i = 1; i <= ((pdf->hdr >> 12) & 0x7); i++) {
        switch((pdf->obj[i - 1] >> 30) & 0x3) {
            case(pdoTypeFixed):
                if(pe_eval_pdo_fixed(pdf->obj[i - 1], power_req)) { ret = i; }
                break;
            case(pdoTypeAugmented):
                if(pe_eval_pdo_augmented(pdf->obj[i - 1], power_req)) { ret = i; }
                break;
            case(pdoTypeBattery):
            case(pdoTypeVariable):
                break;
            default:
                // TODO: Implement error handling
        }
    }
    return ret;
}
bool pe_pdo_is_augmented(uint32_t pdo) {
    return (bool) (((pdo >> 30) & 0x3) == pdoTypeAugmented);
}
bool pe_pdo_is_augmented_idx(pd_frame *pdf, uint pdo_idx) {
    return (pdo_idx > 1) ? pe_pdo_is_augmented(pdf->obj[pdo_idx - 1]) : false;
}
void pe_request_from_srccap_fixed(pd_frame *input_frame, pd_frame *output_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id, uint spec_rev) {
    memset(output_frame, 0, sizeof(pd_frame));
    // Get PDO voltage
    uint pdo_mV = ((input_frame->obj[req_pdo - 1] >> 10) & 0x3FF) * 50;
    // Get PDO maximum current
    uint pdo_mA_max = (input_frame->obj[req_pdo - 1] & 0x3FF) * 10;
    // Determine how much current to request (stay within PDO capabilities)
    uint req_mA = (power_req.mA_max < pdo_mA_max) ? power_req.mA_max : pdo_mA_max;
    // Setup 'Ordered Set' and Message Header (hard-coded values are currently used)
    output_frame->ordered_set = bmcFrameType[typec_pdframe_orderedset_get_idx(input_frame->ordered_set)];
    // Generate Message Header
    output_frame->hdr = (1u << 12)  // # of Data Objects
        | (msg_id & 0x7) << 9       // MsgID
        | (spec_rev & 0x3) << 6     // PD Spec Rev.
        | (uint)dataMsgRequest;     // Request [Power Contract]
    // Generate RDO (Request Data Object)
    output_frame->obj[0] = (req_pdo & 0xF) << 28    // Object position
        | 1u << 25                                  // USB communication capable bit (tell upstream device that we support USB)
        | 1u << 24                                  // No USB suspend bit (request continuing PD contract through USB suspend)
        | ((req_mA / 10) & 0x3FF) << 10             // Operating current
        | ((req_mA / 10) & 0x3FF);                  // Max current
    // Generate CRC32
    output_frame->obj[1] = crc32_pdframe_calc(output_frame);
    //cli_log(DEBUG_LOG, "ReqFixed\n");
    cli_log(DEBUG_LOG, "Request [%u, %umV, %umA] (Fixed)\n", req_pdo, pdo_mV, req_mA);
}
void pe_request_from_srccap_augmented_spr_pps(pd_frame *input_frame, pd_frame *output_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id, uint spec_rev) {
    memset(output_frame, 0, sizeof(pd_frame));
    // Since eval_pdo_augmented has passed we know that the req_pdo.mV_max is within range of this APDO
    // Get PDO maximum current
    uint pdo_mA_max = (input_frame->obj[req_pdo - 1] & 0x7F) * 50;
    uint pdo_mV_max = ((input_frame->obj[req_pdo - 1] >> 17) & 0xFF) * 100;
    // Our PD Sink mode requests the maximum voltage/current both specified in the 'peSinkPowerCriteria' and available from the PD Source
    // We don't request any higher than the Policy Engine requests - but will take less voltage/current as long as we are still above the minimum threshold.
    uint request_mA = (power_req.mA_max > pdo_mA_max) ? pdo_mA_max : power_req.mA_max;
    uint request_mV = (power_req.mV_max > pdo_mV_max) ? pdo_mV_max : power_req.mV_max;
    // Setup 'Ordered Set' and Message Header (hard-coded values are currently used)
    output_frame->ordered_set = bmcFrameType[typec_pdframe_orderedset_get_idx(input_frame->ordered_set)];
    // Generate Message Header
    output_frame->hdr = (1u << 12)  // # of Data Objects
        | (msg_id & 0x7) << 9       // MsgID
        | (spec_rev & 0x3) << 6     // PD Spec Rev.
        | (uint)dataMsgRequest;     // Request [Power Contract]
    // Generate RDO (Request Data Object)
    output_frame->obj[0] = (req_pdo & 0xF) << 28    // Object position
        | 1u << 25                                  // USB communication capable bit (tell upstream device that we support USB)
        | 1u << 24                                  // No USB suspend bit (request continuing PD contract through USB suspend)
        | ((request_mV / 20) & 0xFFF) << 9    // Operating voltage
        | ((request_mA / 50) & 0x7F);               // Operating current
    // Generate CRC32
    output_frame->obj[1] = crc32_pdframe_calc(output_frame);
    //cli_log(DEBUG_LOG, "ReqAugmented %u %u\n", request_mV, request_mA);
    //cli_log(DEBUG_LOG, "Req raw: %X %X %X\n", output_frame->hdr, output_frame->obj[0], output_frame->obj[1]);
    cli_log(DEBUG_LOG, "Request [%u, %umV, %umA] (SPR PPS)\n", req_pdo, request_mV, request_mA);
}
void pe_request_from_srccap_augmented(pd_frame *input_frame, pd_frame *output_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id, uint spec_rev) {
    // Determine which type of request to generate
    switch((input_frame->obj[req_pdo - 1] >> 28) & 0x3) {
        case(pdoTypeAugmentedSprPps):
            pe_request_from_srccap_augmented_spr_pps(input_frame, output_frame, req_pdo, power_req, msg_id, (uint)pdSpecRev3);
            //cli_log(INFO_LOG, "SPR PPS test [%u] MsgID: %u\n", req_pdo, msg_id);
            break;
        case(pdoTypeAugmentedEprAvs):
        case(pdoTypeAugmentedSprAvs):
        case(pdoTypeAugmentedReserved):
            cli_log(WARNING_LOG, "Augmented PDO Type [0x%X] Unimplemented\n", (input_frame->obj[req_pdo - 1] >> 28) & 0x3);
            break;
        default:
            break;
    }
}
void pe_request_from_srccap(pd_frame *input_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id) {
    extern QueueHandle_t mailbox_pe;
    extern QueueHandle_t mailbox_tcpc;
    // Setup output frame structures
    pd_frame *output_frame = pvPortMalloc(sizeof(pd_frame));
    ASSERT_MALLOC(output_frame);
    memset(output_frame, 0, sizeof(pd_frame));
    // Determine which type of request to generate
    switch((input_frame->obj[req_pdo - 1] >> 30) & 0x3) {
        case(pdoTypeFixed):
            pe_request_from_srccap_fixed(input_frame, output_frame, req_pdo, power_req, msg_id, (uint)pdSpecRev3);
            //cli_log(DEBUG_LOG, "PE %X %X\n", output_frame->ordered_set, output_frame->hdr);
            break;
        case(pdoTypeAugmented):
            pe_request_from_srccap_augmented(input_frame, output_frame, req_pdo, power_req, msg_id, (uint)pdSpecRev3);
            break;
        case(pdoTypeBattery):
        case(pdoTypeVariable):
            cli_log(WARNING_LOG, "Base PDO Type [0x%X] Unimplemented\n", (input_frame->obj[req_pdo - 1] >> 30) & 0x3);
            break;
        default:
            break;
    }
    // Send to TCPC (Type-C Port Controller) thread
    powerDeliveryMsg *pd_msg = pvPortMalloc(sizeof(powerDeliveryMsg));
    ASSERT_MALLOC(pd_msg);
    memcpy(&pd_msg->pdf, output_frame, sizeof(pd_frame));
    vPortFree(output_frame);
    //size_t heap_after = xPortGetFreeHeapSize();
    //cli_log(DEBUG_LOG, "FreeRTOS heap free: %u bytes (before: %u)\n", heap_after, heap_before);
    mailerLabel parcel_outgoing = { mailbox_pe, PowerDeliveryMsg, pd_msg };
    xQueueSendToBack(mailbox_tcpc, &parcel_outgoing, 0);
}
/*
void pe_print_pdo_fixed(uint32_t pdo) {
    cli_log(INFO_LOG, "\tVoltage: %u\n", ((pdo >> 10) & 0x3FF) * 50);
    cli_log(INFO_LOG, "\tCurrent: %u\n", (pdo & 0x3FF) * 10);
    cli_log(INFO_LOG, "\tPeak Current: %u\n", (pdo >> 20) & 0x3);
    cli_log(INFO_LOG, "\tEPR Capable: %u\n", (pdo >> 23) & 0x1);
    cli_log(INFO_LOG, "\tUnchunked Support: %u\n", (pdo >> 24) & 0x1);
    cli_log(INFO_LOG, "\tDual-Role Data: %u\n", (pdo >> 25) & 0x1);
    cli_log(INFO_LOG, "\tUSB-Comm Capable: %u\n", (pdo >> 26) & 0x1);
    cli_log(INFO_LOG, "\tUnconstrained Power: %u\n", (pdo >> 27) & 0x1);
    cli_log(INFO_LOG, "\tUSB Suspend Support: %u\n", (pdo >> 28) & 0x1);
    cli_log(INFO_LOG, "\tDual-Role Power: %u\n", (pdo >> 29) & 0x1);
}
void pe_print_source_capabilities(pd_frame *pdf) {
    uint num_objs = (pdf->hdr >> 12) & 0x7;
    for(uint i = 0; i < num_objs; i++) {
        cli_log(INFO_LOG, "PDO %u\n", i);
        uint pdo_base_type = (pdf->obj[i] >> 30) & 0x3;
        cli_log(INFO_LOG, "\tType: %s", pdoBaseTypeNames[pdo_base_type]);
        (pdo_base_type == pdoTypeAugmented) ? cli_log(INFO_LOG, " - %s\n", pdoAugmentedTypeNames[(pdf->obj[i] >> 28) & 0x3]) : cli_log(INFO_LOG, "\n");
        switch(pdo_base_type) {
            case(pdoTypeFixed):
                pe_print_pdo_fixed(pdf->obj[i]);
            case(pdoTypeBattery):
            case(pdoTypeVariable):
            case(pdoTypeAugmented):
                cli_log(WARNING_LOG, "\t[Unsupported]\n");
        }
    }
}
*/
void pe_handle_sop_frame(pd_frame *pdf, peLocalPolicy pe_local_policy, peSinkPowerCriteria pe_sink_criteria, pd_frame *last_srccap_pdf, uint *hdr_msgid, uint *req_pdo) {
    uint frametype_idx = typec_pdframe_get_sop_msg_type(pdf);
    switch(frametype_idx) {
        case(controlMsgGoodCrc):
            cli_log(DEBUG_LOG, "GoodCRC\n");
            break;
        case(controlMsgAccept):
            cli_log(DEBUG_LOG, "Accept\n");
            break;
        case(controlMsgPsReady):
            cli_log(DEBUG_LOG, "PS Ready\n");
            break;
        case(dataMsgSourceCap):
            // Check that we are not in passive mode
            if(!pe_local_policy.passive_mode) {
                // Not in passive mode - generate/send a response to the port-partner
                *req_pdo = optimal_pdo(pdf, pe_sink_criteria);
                pe_request_from_srccap(pdf, *req_pdo, pe_sink_criteria, *hdr_msgid);
                if(memcmp(pdf, last_srccap_pdf, sizeof(pd_frame)) != 0) {
                    memcpy(last_srccap_pdf, pdf, sizeof(pd_frame));
                    // Possibly send to CLI..
                }
            } else {
                // We are in passive mode - log this frame.
                cli_log(DEBUG_LOG, "Source Cap: %X %X %X\n", pdf->hdr, pdf->obj[0], pdf->obj[1]);
            }
            break;
        case(dataMsgRequest):
            cli_log(DEBUG_LOG, "Request frame - Hdr: %X RDO: %X CRC: %X\n", pdf->hdr, pdf->obj[0], pdf->obj[1]);
            break;
        default:
            const char *str_ptr;
            pdMsgType msg_type = (frametype_idx >> 6) & 0x3;
            switch(msg_type) {
                case(controlMsg):
                    str_ptr = pdMsgControlTypeNames[frametype_idx & 0x1F];
                    break;
                case(dataMsg):
                    str_ptr = pdMsgDataTypeNames[frametype_idx & 0x1F];
                    break;
                case(extMsg):
                    str_ptr = pdMsgExtTypeNames[frametype_idx & 0x1F];
                    break;
                default:
                    str_ptr = "Unknown type";
                    break;
            }
            cli_log(WARNING_LOG, "Unimplemented: %s %X %X %X %X -- %u\n", str_ptr, pdf->hdr, pdf->ext_hdr, pdf->obj[0], pdf->obj[1], frametype_idx);
    }
    // Increment msgID
    (*hdr_msgid)++;
    *hdr_msgid &= 0x7;
}

peSinkPowerCriteria pe_sink_criteria = {
    5000,       // mV_min
    12800,      // mV_max
    500,        // mA_min
    2000        // mA_max
};
peLocalPolicy pe_local_policy = {
    pdSpecRev3,     // spec_rev
    true,           // capable_usb_comm
    false,          // capable_epr
    false,          // unchunked_ext_msg
    false,          // passive_mode
};

void policy_engine_task(void *unused_arg) {
    extern peSinkPowerCriteria pe_sink_criteria;
    extern peLocalPolicy pe_local_policy;
    extern QueueHandle_t mailbox_pe;
    mailerLabel parcel_recv;
    pd_frame last_srccap;
    uint sop_msgid = 0;
    uint pdo_idx = 0;
    uint32_t apdo_last_timestamp = 0;
    while(true) {
        if(xQueueReceive(mailbox_pe, (void *) &parcel_recv, 0) == pdPASS) {
            // New data received
            if(parcel_recv.payload_type == PowerDeliveryMsg) {
                powerDeliveryMsg *pd_msg = (powerDeliveryMsg *) parcel_recv.payload_ptr;
                pd_frame *pdf = &pd_msg->pdf;
                switch(typec_pdframe_orderedset_get_idx(pdf->ordered_set)) {
                    case(pdfTypeSop):
                        //cli_log(DEBUG_LOG, "PE %s HDR: %X Obj: %X\n", sopFrameTypeNames[typec_pdframe_orderedset_get_idx(pdf->ordered_set)], pdf->hdr, (pdf->obj)[0]);
                        pe_handle_sop_frame(pdf, pe_local_policy, pe_sink_criteria, &last_srccap, &sop_msgid, &pdo_idx);
                        break;
                    case(pdfTypeSopP):
                    case(pdfTypeSopDp):
                        cli_log(WARNING_LOG, "SOP\' not implemented yet.");
                        break;
                    default:
                }
                vPortFree(pd_msg);
            } else {
                // TODO: Invalid condition...
                cli_log(ERROR_LOG, "Policy Engine: Invalid data received.");
                sleep_ms(5000);
            }
        } else if(pe_pdo_is_augmented_idx(&last_srccap, pdo_idx) && time_us_32() > (apdo_last_timestamp + 5000000)) {
            //cli_log(DEBUG_LOG, "APDO RT\n");
            pe_request_from_srccap(&last_srccap, pdo_idx, pe_sink_criteria, sop_msgid);
            // Increment msgID
            sop_msgid++;
            sop_msgid &= 0x7;
            // Record timestamp
            apdo_last_timestamp = time_us_32();
            /*
            struct mallinfo mi = mallinfo();
            cli_log(DEBUG_LOG, "Total heap size: %d bytes\n", mi.arena);
            cli_log(DEBUG_LOG, "Allocated: %d bytes\n", mi.uordblks);
            cli_log(DEBUG_LOG, "Free: %d bytes\n", mi.fordblks);
            size_t free_rtos_heap = xPortGetFreeHeapSize();
            size_t min_ever_free = xPortGetMinimumEverFreeHeapSize();
            cli_log(DEBUG_LOG, "FreeRTOS heap free: %u bytes (min ever: %u)\n", free_rtos_heap, min_ever_free);
            */
        }
    }
}
