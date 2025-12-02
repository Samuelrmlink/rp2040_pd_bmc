#include "main_i.h"

void debug_pin_toggle(uint8_t pin_num) {
    gpio_set_function(pin_num, GPIO_FUNC_SIO);
    gpio_set_dir(pin_num, true);
    if(gpio_get(pin_num))
        gpio_clr_mask(1 << pin_num); // Drive pin low
    else
        gpio_set_mask(1 << pin_num); // Drive pin high
}
/*
void debug_failover_log(uint32_t level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    // Try normal logging method (may fail if heap memory allocation fails)
    char *buf = NULL;
    loggingMsg *msg = NULL;
    bool sent = false;
    buf = pvPortMalloc(LOG_BUF_SIZE);
    if(buf) {
        msg = pvPortMalloc(sizeof(loggingMsg));
        if(msg) {
            vsnprintf(buf, LOG_BUF_SIZE, fmt, args);
            msg->logLevel = level;
            msg->string   = buf;
            TaskHandle_t current = xTaskGetCurrentTaskHandle();
            QueueHandle_t sender_mailbox = NULL;
            if(current == tskhdl_usb_cli)       sender_mailbox = mailbox_cli;
            else if(current == tskhdl_pd_rxf)   sender_mailbox = mailbox_tcpc;
            else if(current == tskhdl_pd_pe)    sender_mailbox = mailbox_pe;
            mailerLabel parcel = {
                .sender       = sender_mailbox,
                .payload_type = LoggingMsg,
                .payload_ptr  = msg
            };
            if(xQueueSend(mailbox_cli, &parcel, 0) == pdPASS) {
                // Success - fallback logging will not be used
                sent = true;
            }
        }
    }
    // Fallback logging
    if(!sent) {
        // Clean up partial allocation
        if(msg)  vPortFree(msg);
        if(buf)  vPortFree(buf);
        // Restart va_list for printf
        va_end(args);
        va_start(args, fmt);
        const char *prefix = "";
        switch (level) {
            case DEBUG_LOG:   prefix = "\r[DBG] "; break;
            case INFO_LOG:    prefix = "\r[INF] "; break;
            case WARNING_LOG: prefix = "\r[WRN] "; break;
            case ERROR_LOG:   prefix = "\r[ERR] "; break;
        }
        printf("%s", prefix);
        vprintf(fmt, args);
        printf("\r\n");
        fflush(stdout);
    }
    va_end(args);
}
*/
void debug_failed_malloc(const char *file, int line) {
    LOG_FAILOVER(ERROR_LOG, "MALLOC FAILED at %s:%d", file, line);
    size_t free_heap     = xPortGetFreeHeapSize();
    size_t min_ever_free = xPortGetMinimumEverFreeHeapSize();
    LOG_FAILOVER(ERROR_LOG, "FreeRTOS heap → free: %u bytes, minimum ever: %u bytes", free_heap, min_ever_free);
    LOG_FAILOVER(ERROR_LOG, "System halted — out of memory");
    fflush(stdout);
    printf("\r\n\r\n ERROR! \r\n");
    // Visual SOS: blink GP25 (Pico built-in LED) forever at ~2 Hz
    gpio_init(15);
    gpio_set_dir(15, GPIO_OUT);
    while(1) {
        debug_pin_toggle(15);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
