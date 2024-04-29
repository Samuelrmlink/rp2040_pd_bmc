#ifndef _PD_POLICY_ENGINE_H
#define _PD_POLICY_ENGINE_H

typedef enum {
    peMsgPdFrame;
    peMsgRdoPrimary;
    peMsgRdoSecondary;
    // TODO - add more
} peMsgType;

struct policyEngineMsg {
    peMsgType msgType;
    pd_frame *pdf;
    
}



#endif
