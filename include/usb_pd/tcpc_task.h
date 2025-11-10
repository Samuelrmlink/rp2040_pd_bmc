#ifndef _TCPC_TASK_H
#define _TCPC_TASK_H

typedef struct {
    uint accept_sop : 1;    // Accept SOP frames (send to Policy Engine)
    uint accept_sopp : 1;   // Accept SOP' frames (send to Policy Engine)
    uint accept_sopdp : 1;  // Accept SOP" frames (send to Policy Engine)
    uint goodcrc_sop : 1;   // Respond with GoodCRC frame (SOP received)
    uint goodcrc_sopp : 1;  // Respond with GoodCRC frame (SOP' received)
    uint goodcrc_sopdp : 1; // Respond with GoodCRC frame (SOP" received)
    // Mode of operation (passive sniffer, source, sink, etc..)
} tcpcLocalPolicy;


void tcpc_task(void *arg);

#endif
