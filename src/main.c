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
    // Initialize IO & PIO
    stdio_init_all();
    BaseType_t status_task_usb = xTaskCreate(thread_usb, "USB_THREAD", 1024, NULL, 1, &tskhdl_usb_cli);
    assert(status_usb == pdPASS);

    // Setup USB-PD channels
    pdq_rx = bmc_rx_setup();
    bmc_ch0 = bmc_channel0_init();
    BaseType_t status_task_rx_frame = xTaskCreate(thread_rx_process, "PROC_THREAD", 1024, NULL, 1, &tskhdl_pd_rxf);
    assert(status_task_rx_frame == pdPASS);
    irq_set_enabled(bmc_ch0->irq, true);

    // Start the scheduler
    vTaskStartScheduler();
}
