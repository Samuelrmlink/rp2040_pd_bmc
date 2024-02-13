/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "main_i.h"

// Compiler state machine number
#define SM_TX 0
#define SM_RX 1

//Define pins (to be used by PIO for BMC TX/RX)
const uint pin_tx = 9;
const uint pin_rx = 6;

uint32_t *buf1;
PIO pio = pio0;

QueueHandle_t queue_proc = NULL;	// PD Frame process queue
QueueHandle_t queue_print = NULL;	// PD Frame print queue
TaskHandle_t tskhdl_proc = NULL;	// PD Frame process task
TaskHandle_t tskhdl_print = NULL;	// PD Frame print task

TaskHandle_t tskhdl_test = NULL;	// PD Frame process task - TODO - remove

bool bmc_check_during_operation = true;

bmcDecode* bmc_d;
pd_frame lastmsg;
pd_frame *lastmsg_ptr = &lastmsg;
//pd_frame lastsrccap;

void bmc_rx_check() {
    // If PIO RX buffer is not empty
    if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
	// Receive BMC data from PIO interface
	bmc_d->inBuf = pio_sm_get(pio, SM_RX);
	bmc_d->rxTime = time_us_32();// TODO - transition to only using timestamp_us
	lastmsg.timestamp_us = bmc_d->rxTime;
	bmcProcessSymbols(bmc_d, &lastmsg);

	// If frame has a valid CRC
	if(lastmsg.frametype >> 7) {
	    xQueueSendToBack(queue_proc, (void *) &lastmsg_ptr, 0);
	/*
	    // If frame is Source_Capabilies message
	    if((lastmsg.hdr >> 12 & 0x7) && (lastmsg.hdr & 0x1F) == 0x1) {
		memcpy(&lastsrccap, &lastmsg, sizeof(pd_frame));
	    }
	    // Clear lastmsg - TODO: move this to function
	    for(uint8_t i = 0; i < 56; i++) {
		lastmsg.raw_bytes[i] = 0;
	    }
	*/
	}
    }
}
void bmc_rx_cb() {
    bmc_rx_check();
    if(pio_interrupt_get(pio, 0)) {
	pio_interrupt_clear(pio, 0);
    } 
}
const uint32_t bmc_testpayload[] = {	0xAAAAA800, 0xAAAAAAAA, 0x4C6C62AA, 0xEF253E97, 0x2BBDF7A5, 0x56577D55, 0x55555435, 0x55555555,
					0x26363155, 0xD537E4A9, 0x1BBEDF4B, 0x55555554, 0x55555555, 0xABA63631, 0xD2F292B4, 0xF7BDDEFB,
					0xBC997BDE, 0xEF7BDEF7, 0x7BDEF7BD, 0x4EF2FDEF, 0x94DFEF7A, 0x1BC9A529, 0xAAAAAAAA, 0xAAAAAAAA,
					0x94931B18, 0x776AF7F7, 0xAA0DB4AF, 0xAAAAAAAA, 0x18AAAAAA, 0x7A6C98E3, 0xBC99A69A, 0x4DA69AF4,
					0xA69AF7BD, 0x89F7BCAB, 0xF7BCE57B, 0xB7AA25CA, 0xEA26BAD4, 0x269BD4D5, 0x33D4ECAA, 0x7A975969,
					0xAAAAAA0D, 0xAAAAAAAA, 0x98E318AA, 0x6AF7F794, 0x0DB4AF77, 0xAAAAAAAA, 0xAAAAAAAA, 0xA548E318,
					0x592B894F, 0xEF4F5525, 0x0D95373E, 0x55555555, 0x55555555, 0x2E4C718C, 0x93E52EF9, 0x5506AAD5,
					0x55555555, 0x8C555555, 0xFAB6AC71, 0x7DB5B4EE, 0xAAAA86AF, 0xAAAAAAAA, 0x38C62AAA, 0x9BFD4526,
					0x54EBAFDB, 0xAAAAAA83, 0xAAAAAAAA, 0x3A38C62A, 0xBB4F7CBB, 0x83753E73, 0xAAAAAAAA, 0x2AAAAAAA,
					0xA52638C6, 0xBAD2B53C, 0x55436DDD, 0x55555555, 0x63155555, 0x3EA4A71C, 0x5CDCBF6D, 0x555541AA,
					0x55555555, 0x1C631555, 0x737EAD93, 0xAEEEB5AE, 0xAAAAAAA1, 0xAAAAAAAA, 0xCA8E318A, 0xEF2E9F3E,
					0x50D4AF6E, 0x55555555, 0xC5555555, 0x9CA4C718, 0xB96A72E7, }; // 92 32-bit words
void thread_proc(void* unused_arg) {
    printf("Test\n");
    pd_frame *latestmsg;

    while(true) {
    if(xQueueReceive(queue_proc, &latestmsg, 0)) {
	printf("addr: %X - %X - %X\n", latestmsg->hdr, latestmsg, *latestmsg);
	//printf("Data: %X - %u\n", latestmsg->hdr, latestmsg->timestamp_us);
	/*
	    // If frame is Source_Capabilies message
	    if((latestmsg.hdr >> 12 & 0x7) && (latestmsg.hdr & 0x1F) == 0x1) {
		memcpy(&lastsrccap, &latestmsg, sizeof(pd_frame));
	    }
	    // Clear latestmsg - TODO: move this to function
	    for(uint8_t i = 0; i < 56; i++) {
		latestmsg.raw_bytes[i] = 0;
	    }
	*/
    //}
    } else {
	//printf("0q\n");
    }
    sleep_ms(400);
    }
}
void thread_test(void* unused_arg) {
    uint8_t num = 7;
    pd_frame *testframe = malloc(sizeof(pd_frame));
    while(true) {
	//printf("procp\n");
	sleep_ms(1000);
	//printf("test_thread %u - %X\n", uxQueueSpacesAvailable(queue_proc), testframe);
	//bmc_decode_clear(testframe);
	testframe->hdr = num;
	if(xQueueSendToBack(queue_proc, (void *) &testframe, 0)) {
	    printf("Added to queue successfully %X - %X\n", testframe, testframe->hdr);
	} else {
	    printf("Queue is likely full");
	}
	num++;
	sleep_ms(4000);
	//printf("test_thread2 %u\n", uxQueueSpacesAvailable(queue_proc));
    }
}
int main() {
    // Initialize IO & PIO
    stdio_init_all();
    gpio_init(8);
    gpio_set_dir(8, GPIO_OUT);
    buf1 = malloc(256 * 4);
    if(buf1 == NULL) 
	    printf("Error - buf1 is a NULL pointer.");
    for(int i=0;i<=255;i++) {
        buf1[i]=0x00000000;
    }

    // Allocate BMC decoding struct
    bmc_d = malloc(sizeof(bmcDecode));

    /* Initialize TX FIFO*/
    uint offset_tx = pio_add_program(pio, &differential_manchester_tx_program);
    printf("Transmit program loaded at %d\n", offset_tx);
    differential_manchester_tx_program_init(pio, SM_TX, offset_tx, pin_tx, 125.f / (5.3333));
    pio_sm_set_enabled(pio, SM_TX, false);
    /*
    pio_sm_put_blocking(pio, SM_TX, 0);
    pio_sm_put_blocking(pio, SM_TX, 0x0ff0a55a);
    pio_sm_put_blocking(pio, SM_TX, 0x12345678);
    pio_sm_set_enabled(pio, SM_TX, true);*/
    
    /* Initialize RX FIFO */
    uint offset_rx = pio_add_program(pio, &differential_manchester_rx_program);
    printf("Receive program loaded at %d\n", offset_rx);
    differential_manchester_rx_program_init(pio, SM_RX, offset_rx, pin_rx, 125.f / (5.3333));

    pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, bmc_rx_cb);
    irq_set_enabled(PIO0_IRQ_0, true);
    bmc_check_during_operation = false;		// Override - disables this check
 
    uint32_t last_usval;
    uint32_t tmpval;

    // Clear BMC decode data
    bmc_decode_clear(bmc_d);

    // Clear lastmsg - TODO: move this to function
    for(uint8_t i = 0; i < 56; i++) {
	lastmsg.raw_bytes[i] = 0;
    }

    // Setup tasks
    BaseType_t status_task_proc = xTaskCreate(thread_proc, "PROC_THREAD", 128, NULL, 1, &tskhdl_proc);
    //BaseType_t status_task_test = xTaskCreate(thread_test, "TEST_THREAD", 128, NULL, 1, &tskhdl_test);
    //BaseType_t status_task_print = xTaskCreate(thread_print, "PRINT_TASK", 128, NULL, 1, &tskhdl_print);

    if(status_task_proc == pdPASS) {
	// Setup the queues
	queue_proc = xQueueCreate(4, sizeof(pd_frame));
	queue_print = xQueueCreate(4, sizeof(pd_frame));
	
	// Start the scheduler
	vTaskStartScheduler();
    } else {
	printf("Unable to start task scheduler.\n");
    }


    /* TEST CASE
    sleep_ms(4);
    bmc_d->rxTime = time_us_32();
    // Note - 26, 15 is the source_capabilies message
    bmc_testfill(bmc_testpayload, 92, bmc_d, &lastmsg, &lastsrccap, false);
    printf("procStage: %u\n", bmc_d->procStage);
    printf("Time us: %u\n", time_us_32() - bmc_d->rxTime);
    printf("sopType: %X\n", lastmsg.frametype);
    printf("msgHdr: %X\n", lastmsg.hdr);
    printf("Obj0-10: %X %X %X %X %X %X %X %X %X %X %X\n", lastmsg.obj[0], lastmsg.obj[1], lastmsg.obj[2], lastmsg.obj[3], lastmsg.obj[4], lastmsg.obj[5], lastmsg.obj[6], lastmsg.obj[7], lastmsg.obj[8], lastmsg.obj[9], lastmsg.obj[10]);
    printf("CRC32: %X calc: %X\n", bmc_d->crcTmp, crc32_pdframe_calc(&lastmsg));
    sleep_ms(3);






    while(true) {
    //printf("procStage: %u\n", bmc_d->procStage);
	if(bmc_d->rxTime != last_usval) {
	    last_usval = bmc_d->rxTime;
	    printf("rxTime: %u    %X,%X,%X\n", bmc_d->rxTime, bmc_d->procStage, bmc_d->procSubStage, bmc_d->procBuf);
	}
	if(!(lastsrccap.frametype >> 6 & 0x1)) {
	    lastsrccap.frametype |= (0x1 << 6);
	    printf("SrccapHdr: %X\n", lastsrccap.hdr);
	}
    }
    */
}
