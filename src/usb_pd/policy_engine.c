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
            //printf("EPR AVS\n");
            break;
        case(pdoTypeAugmentedSprAvs):
            //printf("SPR AVS\n");
            break;
        default:
            //printf("Invalid/Reserved\n");
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
    // Get PDO maximum current
    uint mA_max = (input_frame->obj[req_pdo - 1] & 0x3FF) * 10;
    // Replace maximum current value if requested current is lower
    mA_max = (power_req.mA_max < mA_max) ? power_req.mA_max : mA_max;
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
        | ((mA_max / 10) & 0x3FF) << 10             // Operating current
        | ((mA_max / 10) & 0x3FF);                  // Max current
    // Generate CRC32
    output_frame->obj[1] = crc32_pdframe_calc(output_frame);
    printf("ReqFixed\n");
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
    printf("ReqAugmented %u %u\n", request_mV, request_mA);
    printf("Req raw: %X %X %X\n", output_frame->hdr, output_frame->obj[0], output_frame->obj[1]);
}
void pe_request_from_srccap_augmented(pd_frame *input_frame, pd_frame *output_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id, uint spec_rev) {
    // Determine which type of request to generate
    switch((input_frame->obj[req_pdo - 1] >> 28) & 0x3) {
        case(pdoTypeAugmentedSprPps):
            pe_request_from_srccap_augmented_spr_pps(input_frame, output_frame, req_pdo, power_req, msg_id, (uint)pdSpecRev3);
            break;
        case(pdoTypeAugmentedEprAvs):
        case(pdoTypeAugmentedSprAvs):
        case(pdoTypeAugmentedReserved):
            printf("Unimplemented\n");
            break;
        default:
            break;
    }
}
void pe_request_from_srccap(pd_frame *input_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id) {
    extern QueueHandle_t mailbox_pe;
    extern QueueHandle_t mailbox_tcpc;
    // Setup output frame structures
    pd_frame *output_frame = malloc(sizeof(pd_frame));
    memset(output_frame, 0, sizeof(pd_frame));
    // Determine which type of request to generate
    switch((input_frame->obj[req_pdo - 1] >> 30) & 0x3) {
        case(pdoTypeFixed):
            pe_request_from_srccap_fixed(input_frame, output_frame, req_pdo, power_req, msg_id, (uint)pdSpecRev3);
            //printf("PE %X %X\n", output_frame->ordered_set, output_frame->hdr);
            break;
        case(pdoTypeAugmented):
            pe_request_from_srccap_augmented(input_frame, output_frame, req_pdo, power_req, msg_id, (uint)pdSpecRev3);
            break;
        case(pdoTypeBattery):
        case(pdoTypeVariable):
            printf("Unimplemented\n");
            break;
        default:
            break;
    }
    // Send to TCPC (Type-C Port Controller) thread
    powerDeliveryMsg *pd_msg = malloc(sizeof(powerDeliveryMsg));
    memcpy(&pd_msg->pdf, output_frame, sizeof(pd_frame));
    free(output_frame);
    mailerLabel parcel_outgoing = { mailbox_pe, PowerDeliveryMsg, pd_msg };
    xQueueSendToBack(mailbox_tcpc, &parcel_outgoing, 0);
    // Debugging - log to serial
    //printf("Req: %X %X %X\n", output_frame->hdr, output_frame->obj[0], output_frame->obj[1]);
}

void pe_handle_sop_frame(pd_frame *pdf, peSinkPowerCriteria pe_sink_criteria, pd_frame *last_srccap_pdf, uint *hdr_msgid, uint *req_pdo) {
    uint frametype_idx = typec_pdframe_get_sop_msg_type(pdf);
    switch(frametype_idx) {
        case(controlMsgGoodCrc):
            printf("GoodCRC\n");
            break;
        case(controlMsgAccept):
            printf("Accept\n");
            break;
        case(controlMsgPsReady):
            printf("PS Ready\n");
            break;
        case(dataMsgSourceCap):
            *req_pdo = optimal_pdo(pdf, pe_sink_criteria);
            pe_request_from_srccap(pdf, *req_pdo, pe_sink_criteria, *hdr_msgid);
            memcpy(last_srccap_pdf, pdf, sizeof(pd_frame));
            break;
        case(dataMsgRequest):
            printf("Request frame - Hdr: %X RDO: %X CRC: %X\n", pdf->hdr, pdf->obj[0], pdf->obj[1]);
            break;
        default:
            printf("Unimplemented: %s\n", pdMsgTypeNames[frametype_idx]);
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

void policy_engine_task(void *unused_arg) {
    extern peSinkPowerCriteria pe_sink_criteria;
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
                        //printf("PE %s HDR: %X Obj: %X\n", sopFrameTypeNames[typec_pdframe_orderedset_get_idx(pdf->ordered_set)], pdf->hdr, (pdf->obj)[0]);
                        pe_handle_sop_frame(pdf, pe_sink_criteria, &last_srccap, &sop_msgid, &pdo_idx);
                        break;
                    case(pdfTypeSopP):
                    case(pdfTypeSopDp):
                        printf("SOP\' not implemented yet.");
                        break;
                    default:
                }
                free(pd_msg);
            } else {
                // TODO: Invalid condition...
//                printf("K");
                sleep_ms(5000);
            }
        } else if(pe_pdo_is_augmented_idx(&last_srccap, pdo_idx) && time_us_32() > (apdo_last_timestamp + 5000000)) {
            printf("APDO RT\n");
            pe_request_from_srccap(&last_srccap, pdo_idx, pe_sink_criteria, sop_msgid);
            // Increment msgID
            sop_msgid++;
            sop_msgid &= 0x7;
            // Record timestamp
            apdo_last_timestamp = time_us_32();
        }
    }
}
