#include "main_i.h"
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif


uint32_t mcu_reg_get_uint(keyData* key, bool use_multiplier) {
    extern uint32_t reg;
    uint32_t ret_val = (config_reg[key->Bv.regNum] >> (key->Bv.lsbOffset)) & (0xFFFFFFFF >> (31 - key->Bv.msbOffset));
    if(use_multiplier) {
        ret_val *= key->Bv.valMultp;
    }
    assert(ret_val <= key->Bv.maxValue);
    assert(ret_val >= key->Bv.minValue);
    return ret_val;
}


#ifdef __cplusplus
}
#endif