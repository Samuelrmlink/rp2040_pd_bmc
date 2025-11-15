#ifndef _POLICY_ENGINE_H
#define _POLICY_ENGINE_H

typedef struct {
    uint mV_min;
    uint mV_max;
    uint mA_min;
    uint mA_max;
} peSinkPowerCriteria;

typedef struct {
    uint spec_rev : 2;
    uint capable_usb_comm : 1;
    uint capable_epr : 1;
    uint unchunked_ext_msg : 1;
} peLocalPolicy;

#endif
