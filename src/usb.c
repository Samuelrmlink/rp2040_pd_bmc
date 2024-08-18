#include "main_i.h"

static void thread_usb(void* unused_arg) {
    while(true) {
        cli_work();
    }
}
void usb_init(void) {
    TaskHandle_t handle_usb = NULL;
    BaseType_t status_usb = xTaskCreate(thread_usb, "usb_thread", 1024, NULL, 1, &handle_usb);
    assert(status_usb == pdPASS);
}
