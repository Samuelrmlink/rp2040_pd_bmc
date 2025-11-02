#ifndef _TCPC_TASK_H
#define _TCPC_TASK_H

typedef struct {
    uint accept_sop : 1;
    uint accept_sopp : 1;
    uint accept_sopdp : 1;
    // Mode of operation (passive sniffer, source, sink, etc..)
} tcpcLocalPolicy;


void tcpc_task(void *arg);

#endif
