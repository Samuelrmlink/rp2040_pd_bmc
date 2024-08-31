#ifndef _PD_POLICY_ENGINE_H
#define _PD_POLICY_ENGINE_H

typedef enum {
    peMsgPdFrameIn,
    peMsgPdFrameOut,
    peMsgRdoPrimary,
    peMsgRdoSecondary
    // TODO - add more
} peMsgType;

typedef struct peOptions peOptions;
struct peOptions {
    bool analyzer_mode;
    pdo_accept_criteria power_req;
};

struct policyEngineMsg {
    peMsgType msgType;
    pd_frame *pdf;
    txFrame *txf;
};
typedef struct policyEngineMsg policyEngineMsg;



#endif
