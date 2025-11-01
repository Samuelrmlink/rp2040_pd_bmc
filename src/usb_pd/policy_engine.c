#include "main_i.h"

void policy_engine_task(void *unused_arg) {
    extern QueueHandle_t mailbox_pe;
    mailerLabel parcel_recv;
    while(true) {
        if(xQueueReceive(mailbox_pe, (void *) &parcel_recv, 0) == pdPASS) {
            // New data received
            if(parcel_recv.payload_type == PowerDeliveryMsg) {
                powerDeliveryMsg *pd_msg = (powerDeliveryMsg *) parcel_recv.payload_ptr;
                pd_frame *pdf = &pd_msg->pdf;
//                printf("PE %X HDR: %X\n", typec_pdframe_orderedset_get_idx(pdf->ordered_set), pdf->hdr);
                printf("PE %s HDR: %X Obj: %X\n", sopFrameTypeNames[typec_pdframe_orderedset_get_idx(pdf->ordered_set)], pdf->hdr, (pdf->obj)[0]);
                free(pd_msg);
            } else {
                // TODO: Invalid condition...
//                printf("K");
                sleep_ms(5000);
            }
        }
    }
}
