/*
 *  rp2040_pd_bmc
 *  Copyright (C) 2024 Samuel Hedrick 
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "main_i.h"


// Task Handles (Thread Handles)
TaskHandle_t tskhdl_usb_cli = NULL; // Task handle: USB CDC-ACM w/CLI
TaskHandle_t tskhdl_pd_rxf = NULL;	// Task handle: RX frame receiver
TaskHandle_t tskhdl_pd_pol = NULL;  // Task handle: USB-PD policy engine

// Queues
QueueHandle_t queue_pc_in = NULL;   // Queue: Port Controller Input
QueueHandle_t queue_pe_in = NULL;   // Queue: Policy Engine Input

// Channel structures
bmcRx *pdq_rx;
bmcTx *tx;

// USB thread
static void thread_usb(void* unused_arg) {
    while(true) {
        cli_work();
    }
}

int main() {
    extern bmcChannels *bmc_ch;

    // Initialize IO & PIO
    stdio_init_all();
    BaseType_t status_task_usb = xTaskCreate(thread_usb, "USB_THREAD", 1024, NULL, 1, &tskhdl_usb_cli);
    assert(status_task_usb == pdPASS);

    // Setup USB-PD channels
    pdq_rx = bmc_rx_setup();
    tx = bmc_tx_setup();
    bmc_ch = bmc_channels_alloc(4);
    bool ch_reg = bmc_channel_register(bmc_ch, pio0, 0, 1, PIO0_IRQ_0, 6, 9, 10, 26);
    //bool ch_reg = bmc_channel_register(bmc_ch, pio0, 0, 1, PIO0_IRQ_0, 7, 11, 12, 27);
    assert(ch_reg);

    queue_pc_in = xQueueCreate(3, sizeof(pd_frame));
    queue_pe_in = xQueueCreate(4, sizeof(pd_frame));
    BaseType_t status_task_rx_frame = xTaskCreate(thread_pd_portctrl, "PD_PORTCTRL", 1024, NULL, 1, &tskhdl_pd_rxf);
    BaseType_t status_task_pe = xTaskCreate(thread_pd_policy_engine, "PD_POLICY", 2048, NULL, 1, &tskhdl_pd_pol);
    assert(status_task_rx_frame == pdPASS);
    irq_set_enabled((bmc_ch->chan)[0].irq, true);
    // Start the scheduler
    vTaskStartScheduler();
}
