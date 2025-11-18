#ifndef _MAILBOX_IPC_H
#define _MAILBOX_IPC_H

#include "main_i.h"

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

void policy_engine_task(void *unused_arg);

#endif
