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

bool bmc_check_during_operation = true;

bmcDecode* bmc_d;

void bmc_rx_check() {
    if(!pio_sm_is_rx_fifo_empty(pio, SM_RX)) {
	bmc_d->inBuf = pio_sm_get(pio, SM_RX);
	bmc_d->rxTime = time_us_32();
    }
}
void bmc_rx_cb() {
    bmc_rx_check();
    if(pio_interrupt_get(pio, 0)) {
	pio_interrupt_clear(pio, 0);
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
    pd_frame lastmsg;

    // Clear BMC decode data - TODO: move this to function
    bmc_d->procStage = 0;
    bmc_d->procSubStage = 0;
    bmc_d->inBuf = 0;
    bmc_d->procBuf = 0;
    bmc_d->pOffset = 0;
    bmc_d->rxTime = 0;
    bmc_d->crcTmp = 0;
    // Clear lastmsg - TODO: move this to function
    for(uint8_t i = 0; i < 56; i++) {
	lastmsg.raw_bytes[i] = 0;
    }

    /* TEST CASE */
    sleep_ms(4);
    bmc_d->rxTime = time_us_32();
    bmc_d->inBuf = 0xAAAAAAAA;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed0: %X, %X, %X, %X\n", bmc_d->inBuf, lastmsg.frametype, bmc_d->pOffset, bmc_d->procStage);
    bmc_d->inBuf = 0xAAAAAAAA;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed1: %X, %X, %X, %X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage);
    bmc_d->inBuf = 0x4C6C62AA;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed2: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0xEF253E97;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed3: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0x2BBDF7A5;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed4: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0x56577D55;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed5: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0x55555435;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed6: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0x55555555;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed7: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0x26363155;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed8: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
    bmc_d->inBuf = 0xD537E4A9;
    bmcProcessSymbols(bmc_d, &lastmsg);
    //printf("DBGusPassed9: %X, %X, %X, %X:%X\n", bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);

    printf("Time us: %u\n", time_us_32() - bmc_d->rxTime);
    printf("sopType: %X\n", lastmsg.frametype);
    printf("msgHdr: %X\n", lastmsg.hdr);
    printf("Obj0-10: %X %X %X %X %X %X %X %X %X %X %X\n", lastmsg.obj[0], lastmsg.obj[1], lastmsg.obj[2], lastmsg.obj[3], lastmsg.obj[4], lastmsg.obj[5], lastmsg.obj[6], lastmsg.obj[7], lastmsg.obj[8], lastmsg.obj[9], lastmsg.obj[10]);
    printf("CRC32: %X calc: %X\n", bmc_d->crcTmp, crc32_pdframe_calc(&lastmsg));
    sleep_ms(3);





    while(true) {
	if(bmc_d->rxTime != last_usval) {
	    tmpval = bmc_d->rxTime - last_usval;
	    last_usval = bmc_d->rxTime;

	    bmcProcessSymbols(bmc_d, &lastmsg);
	    printf("usPassed: %u, %X, %X, %X, %X\n", tmpval, bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage);
	}

	//Clear the CDC-ACM output
	if(time_us_32() - last_usval > 200) {
	    sleep_us(3);
	}
    }
}
