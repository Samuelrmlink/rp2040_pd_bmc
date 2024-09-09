/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "main_i.h"

// Queues
//QueueHandle_t queue_rx_pio = NULL;		// PD Frame:	Raw PIO output pipeline (1st stage)
//QueueHandle_t queue_rx_validFrame = NULL;	// PD Frame:	Valid USB-PD frame	(2nd stage)
QueueHandle_t queue_policy = NULL;		// PD Frame:	Input to policy thread	(3rd stage)

// Task Handles (Thread Handles)
TaskHandle_t tskhdl_pd_rxf = NULL;	// Task handle: RX frame receiver
TaskHandle_t tskhdl_pd_pol = NULL;	// Task handle: USB-PD policy

/*
pe_outbound_pdf(txFrame *tx_out) {
    policyEngineMsg msg;
    msg.msgType = peMsgPdFrameOut;
    msg.txf = tx_out;
    xQueueSendToBack(queue_policy, &msg, portMAX_DELAY);
}
*/
/*
void thread_rx_policy(void *unused_arg) {
    // TODO - implement policy states for both current/perferred Power Sink/Source, Data UFP/DFP roles
    policyEngineMsg peMsg;
    pd_frame latestSrcCap, latestReqDataObj;
    pd_frame_clear(&latestSrcCap);
    pd_frame_clear(&latestReqDataObj);
    bool analyzer_mode = false;
    pdo_accept_criteria power_req = {
        .mV_min = 5000,
        .mV_max = 10240,
        .mA_min = 0,
        .mA_max = 2000
    };
    uint8_t tmpindex;

    while(true) {
	xQueueReceive(queue_policy, &peMsg, portMAX_DELAY);
        if(peMsg.msgType == peMsgPdFrameIn) {
            // Analyzer Mode
            if(analyzer_mode) {
    	    printf("%u - %s Header: %X %s\n", peMsg.pdf->timestamp_us, sopFrameTypeNames[peMsg.pdf->frametype & 0x7], peMsg.pdf->hdr, pdMsgTypeNames[pdf_get_sop_msg_type(peMsg.pdf)]);
            // PD Sink Mode - TODO: add more/better operational modes
    	    } else {
                // Prep the TX response structure
                txFrame txf;
                txf.pdf = malloc(sizeof(pd_frame));

                // Generate and transmit GoodCRC (if applicable)
                if((pdf_get_sop_msg_type(peMsg.pdf) != controlMsgGoodCrc) && check_sop_type(pdfTypeSop, peMsg.pdf)) {
                    pdf_generate_goodcrc(peMsg.pdf, &txf);
                    pe_outbound_pdf(&txf);
                }

                // If Source Capabilities
                if(pdf_get_sop_msg_type(peMsg.pdf) == dataMsgSourceCap) {
                    memcpy(&latestSrcCap, peMsg.pdf, sizeof(pd_frame));
                    tmpindex = optimal_pdo(&latestSrcCap, power_req);
                    if(!tmpindex) {     // If no acceptable PDO is found - just request the first one (always 5v)
                        tmpindex = 1;
                        pdf_request_from_srccap(&latestSrcCap, &txf, tmpindex, power_req);
                    } else {
                        pdf_request_from_srccap(&latestSrcCap, &txf, tmpindex, power_req);
                    }
                    pe_outbound_pdf(&txf);

                    latestSrcCap.hdr = 0;
                }

            }

            // Cleanup - wipe the pd_frame received (whether we've done anything with it or not)
            free(peMsg.pdf);
        } else if(peMsg.msgType == peMsgPdFrameOut) {
            pe_transmit(peMsg.txf, bmc_ch0);
        }
    }
}
*/
bmcRx *pdq_rx;

int main() {
    // Initialize IO & PIO
    stdio_init_all();
    bmc_ch0 = bmc_channel0_init();
    pdq_rx = bmc_rx_setup();
    pd_frame_clear(&((pdq_rx->pdfPtr)[0])); // TODO - currently only clears the first object
    usb_init();

    //BaseType_t status_task_printtest = xTaskCreate(thread_print_test, "TEST_THREAD", 1024, NULL, 2, &tskhdl_pd_pol);


    // Setup tasks
//    BaseType_t status_task_rx_frame = xTaskCreate(thread_rx_process, "PROC_THREAD", 1024, NULL, 1, &tskhdl_pd_rxf);
//    BaseType_t status_task_policy = xTaskCreate(thread_rx_policy, "POLICY_THREAD", 1024, NULL, 2, &tskhdl_pd_pol);

/*
    if(status_task_rx_frame == pdPASS) {
	// Setup the queues
//	queue_rx_pio = xQueueCreate(1000, sizeof(rx_data));
//	queue_rx_validFrame = xQueueCreate(10, sizeof(pd_frame));
//	queue_policy = xQueueCreate(10, sizeof(policyEngineMsg));
	*/irq_set_enabled(bmc_ch0->irq, true);
	
	// Start the scheduler
	vTaskStartScheduler();/*
    } else {
	printf("Unable to start task scheduler.\n");
    }
*/
}
