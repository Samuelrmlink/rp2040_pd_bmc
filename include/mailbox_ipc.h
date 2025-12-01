#ifndef _MAILBOX_IPC_H
#define _MAILBOX_IPC_H

#include "main_i.h"

// Task Handles (Thread Handles)
extern TaskHandle_t tskhdl_usb_cli; // Task handle: USB CDC-ACM w/CLI
extern TaskHandle_t tskhdl_pd_rxf;  // Task handle: RX frame receiver
extern TaskHandle_t tskhdl_pd_pe;

// Mailboxes (IPC mechanism)
extern QueueHandle_t mailbox_tcpc;
extern QueueHandle_t mailbox_pe;
extern QueueHandle_t mailbox_cli;



enum PayloadTypes {
    PowerDeliveryMsg,
    LoggingMsg,
};

typedef struct {
    QueueHandle_t sender;
    uint32_t payload_type;
    void *payload_ptr;
} mailerLabel;

typedef struct {
    pd_frame pdf;
} powerDeliveryMsg;

typedef struct {
    enum {
        DEBUG_LOG,
        INFO_LOG,
        WARNING_LOG,
        ERROR_LOG
    } logLevel;
    char *string;
} loggingMsg;

// Universal logging function (callable from any FreeRTOS task)

void policy_engine_task(void *unused_arg);

#endif
