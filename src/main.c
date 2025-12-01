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
TaskHandle_t tskhdl_usb_cli = NULL;    // Task handle: USB CDC-ACM w/CLI
TaskHandle_t tskhdl_pd_rxf = NULL;     // Task handle: RX frame receiver
TaskHandle_t tskhdl_pd_pe = NULL;

// Mailboxes (IPC mechanism)
QueueHandle_t mailbox_tcpc = NULL;
QueueHandle_t mailbox_pe = NULL;
QueueHandle_t mailbox_cli = NULL;

// USB thread
static void thread_usb(void* unused_arg) {
    while(true) {
        cli_work();
    }
}

int main() {
    // Initialize IO
    stdio_init_all();
    BaseType_t status_task_usb = xTaskCreate(thread_usb, "USB_THREAD", 1024, NULL, 1, &tskhdl_usb_cli);
    mailbox_cli = xQueueCreate(40, sizeof(mailerLabel));
    assert(status_task_usb == pdPASS);

    // Initialize TCPC (Type-C Port Controller) Task
    BaseType_t status_task_tcpc = xTaskCreate(tcpc_task, "TCPC_THREAD", 2048, NULL, 5, &tskhdl_pd_rxf);
    mailbox_tcpc = xQueueCreate(4, sizeof(mailerLabel));

    // Initialize Policy Engine Task
    BaseType_t status_task_pe = xTaskCreate(policy_engine_task, "PE_TASK", 2048, NULL, 2, &tskhdl_pd_pe);
    mailbox_pe = xQueueCreate(4, sizeof(mailerLabel));

    // Start the scheduler
    vTaskStartScheduler();
}
