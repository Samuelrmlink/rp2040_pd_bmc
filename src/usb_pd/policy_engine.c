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
            printf("SPR PPS\n");
            // Get SPR PPS PDO values
            uint32_t pdo_mV_max = ((pdo_obj >> 17) & 0xFF) * 100;
            uint32_t pdo_mV_min = ((pdo_obj >> 8) & 0xFF) * 100;
            uint pdo_mA_max = (pdo_obj & 0x7F) * 50;
            // Compare PDO values with the criteria
            if(pdo_mV_max >= req.mV_max && pdo_mV_max >= req.mV_min && pdo_mA_max >= req.mA_min) { return true; }
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
}
void pe_request_from_srccap_augmented_spr_pps(pd_frame *input_frame, pd_frame *output_frame, uint req_pdo, peSinkPowerCriteria power_req, uint msg_id, uint spec_rev) {
    memset(output_frame, 0, sizeof(pd_frame));
    // Since eval_pdo_augmented has passed we know that the req_pdo.mV_max is within range of this APDO
    // Get PDO maximum current
    uint pdo_mA_max = (input_frame->obj[req_pdo - 1] & 0x7F) * 50;
    // Any modification of the current value does not leave this function
    if(power_req.mA_max > pdo_mA_max) {
        // Max current requested is higher than provided by the charger - just take what we can get
        power_req.mA_max = pdo_mA_max;
    }
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
        | ((power_req.mV_max / 20) & 0xFFF) << 9    // Operating voltage
        | ((pdo_mA_max / 50) & 0x7F);               // Operating current
    // Generate CRC32
    output_frame->obj[1] = crc32_pdframe_calc(output_frame);
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

void pe_handle_sop_frame(pd_frame *pdf, peSinkPowerCriteria pe_sink_criteria, pd_frame *last_srccap_pdf) {
    uint frametype_idx = typec_pdframe_get_sop_msg_type(pdf);
    switch(frametype_idx) {
        case(controlMsgGoodCrc):
            printf("GoodCRC\n");
            break;
        case(controlMsgAccept):
            printf("Accept\n");
            break;
        case(controlMsgPsReady):
            printf("PS Ready :D");
            break;
        case(dataMsgSourceCap):
            uint req_pdo = optimal_pdo(pdf, pe_sink_criteria);
            pe_request_from_srccap(pdf, req_pdo, pe_sink_criteria, 0);
            break;
        default:
            printf("Unimplemented: %s\n", pdMsgTypeNames[frametype_idx]);
    }
}

peSinkPowerCriteria pe_sink_criteria = {
    5000,       // mV_min
    11000,      // mV_max
    500,        // mA_min
    2000        // mA_max
};

void policy_engine_task(void *unused_arg) {
    extern peSinkPowerCriteria pe_sink_criteria;
    extern QueueHandle_t mailbox_pe;
    mailerLabel parcel_recv;
    pd_frame last_srccap;
    while(true) {
        if(xQueueReceive(mailbox_pe, (void *) &parcel_recv, 0) == pdPASS) {
            // New data received
            if(parcel_recv.payload_type == PowerDeliveryMsg) {
                powerDeliveryMsg *pd_msg = (powerDeliveryMsg *) parcel_recv.payload_ptr;
                pd_frame *pdf = &pd_msg->pdf;
                switch(typec_pdframe_orderedset_get_idx(pdf->ordered_set)) {
                    case(pdfTypeSop):
                        //printf("PE %s HDR: %X Obj: %X\n", sopFrameTypeNames[typec_pdframe_orderedset_get_idx(pdf->ordered_set)], pdf->hdr, (pdf->obj)[0]);
                        pe_handle_sop_frame(pdf, pe_sink_criteria, &last_srccap);
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
        }
    }
}
