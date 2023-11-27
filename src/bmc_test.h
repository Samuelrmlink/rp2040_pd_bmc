#ifndef _BMC_TEST_H
#define _BMC_TEST_H

#include "bmc_decode.h"

void bmc_testfill(uint32_t *data_ptr, uint16_t data_count, bmcDecode* bmc_d, pd_frame* pdf, bool debug);

#endif
