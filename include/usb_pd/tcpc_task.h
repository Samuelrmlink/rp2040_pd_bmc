#ifndef _TCPC_TASK_H
#define _TCPC_TASK_H

typedef struct {
    uint accept_sop : 1;
    uint accept_sopp : 1;
    uint accept_sopdp : 1;
    // Mode of operation (passive sniffer, source, sink, etc..)
} tcpcLocalPolicy;

typedef struct {
    uint mV_min;
    uint mV_max;
    uint mA_min;
    uint mA_max;
} tcpcSinkPowerCriteria;

void tcpc_task(void *arg);

#endif
