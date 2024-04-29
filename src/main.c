/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "main_i.h"

// Queues
QueueHandle_t queue_rx_pio = NULL;		// PD Frame:	Raw PIO output pipeline (1st stage)
QueueHandle_t queue_rx_validFrame = NULL;	// PD Frame:	Valid USB-PD frame	(2nd stage)
QueueHandle_t queue_policy = NULL;		// PD Frame:	Input to policy thread	(3rd stage)

// Task Handles (Thread Handles)
TaskHandle_t tskhdl_pd_rxf = NULL;	// Task handle: RX frame receiver
TaskHandle_t tskhdl_pd_pol = NULL;	// Task handle: USB-PD policy


void thread_rx_policy(void *unused_arg) {
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    pd_frame *cFrame = malloc(sizeof(pd_frame));
    txFrame *txf = malloc(sizeof(txFrame));
    txf->pdf = malloc(sizeof(pd_frame));
    pd_frame latestSrcCap, latestReqDataObj;
    pd_frame_clear(&latestSrcCap);
    pd_frame_clear(&latestReqDataObj);
    uint32_t timestamp_now = 0;
    PDMessageType msgType;
    bool analyzer_mode = false;
    bool check_srccap = false;
    uint16_t req_mvolt = 9000; // Very basic voltage request - TODO: remove
    uint8_t tmpindex;

    while(true) {
	xQueueReceive(queue_policy, cFrame, 10);
	timestamp_now = time_us_32();
	//printf("%u:%u - %s Header: %X %s | %X:%X:%X\n", cFrame->timestamp_us, (timestamp_now - cFrame->timestamp_us), sopFrameTypeNames[cFrame->frametype & 0x7], cFrame->hdr, pdMsgTypeNames[pdf_get_sop_msg_type(cFrame)], cFrame->obj[0], cFrame->obj[1], cFrame->obj[2]);
	if(is_crc_good(cFrame) && (pdf_get_sop_msg_type(cFrame) != controlMsgGoodCrc) && is_sop_frame(cFrame) && !analyzer_mode && (cFrame->hdr != latestSrcCap.hdr)) {
	    // Send a GoodCRC response frame
	    pdf_generate_goodcrc(cFrame, txf);
        //txf->out[0] |= 0x2;
	    pdf_transmit(txf, bmc_ch0);

	    // If Source Capabilities
        if(pdf_get_sop_msg_type(cFrame) == dataMsgSourceCap) {
            memcpy(&latestSrcCap, cFrame, sizeof(pd_frame));
            check_srccap = true;
        }
    // Clear the cFrame variable
    pd_frame_clear(cFrame);
    free(txf->out);
	} else if(latestSrcCap.hdr && check_srccap) {
        check_srccap = false;
        tmpindex = optimal_pdo(&latestSrcCap, req_mvolt);
        if(tmpindex) {
            pdf_request_from_srccap(&latestSrcCap, txf, tmpindex);
            printf("%X:%u:%X %X\n", txf->pdf, txf->num_u32, txf->pdf->hdr, txf->pdf->obj[0]);
            pdf_transmit(txf, bmc_ch0);
            printf("op:%u\n", optimal_pdo(&latestSrcCap, req_mvolt));
        }
    }
    }
}
int main() {
    // Initialize IO & PIO
    stdio_init_all();
    bmc_ch0 = bmc_channel0_init();

    // Setup tasks
    BaseType_t status_task_rx_frame = xTaskCreate(thread_rx_process, "PROC_THREAD", 1024, NULL, 1, &tskhdl_pd_rxf);
    BaseType_t status_task_policy = xTaskCreate(thread_rx_policy, "POLICY_THREAD", 1024, NULL, 2, &tskhdl_pd_pol);

    if(status_task_rx_frame == pdPASS) {
	// Setup the queues
	queue_rx_pio = xQueueCreate(1000, sizeof(rx_data));
	queue_rx_validFrame = xQueueCreate(10, sizeof(pd_frame));
	queue_policy = xQueueCreate(10, sizeof(pd_frame));
/*
    // Test raw frame generation - TODO: remove
    pd_frame *cFrame = malloc(sizeof(pd_frame));
    txFrame *txf = malloc(sizeof(txFrame));
    txf->pdf = malloc(sizeof(pd_frame));
    pdf_generate_source_capabilities_basic(cFrame, txf);
    //printf("txlow: %u\n", bmc_ch0->tx_low);
    pdf_transmit(txf, bmc_ch0);
    busy_wait_us(500000);
*/
    irq_set_enabled(bmc_ch0->irq, true);
	
	// Start the scheduler
	vTaskStartScheduler();
    } else {
	printf("Unable to start task scheduler.\n");
    }
}
