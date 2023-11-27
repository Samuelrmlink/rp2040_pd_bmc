#include "main_i.h"

/* - TODO - this is currently also in the main.c source file (which is the copy being used)
const uint32_t bmc_testpayload[] = {	0xAAAAA800, 0xAAAAAAAA, 0x4C6C62AA, 0xEF253E97, 0x2BBDF7A5, 0x56577D55, 0x55555435, 0x55555555,
					0x26363155, 0xD537E4A9, 0x1BBEDF4B, 0x55555554, 0x55555555, 0xABA63631, 0xD2F292B4, 0xF7BDDEFB,
					0xBC997BDE, 0xEF7BDEF7, 0x7BDEF7BD, 0x4EF2FDEF, 0x94DFEF7A, 0x1BC9A529, 0xAAAAAAAA, 0xAAAAAAAA,
					0x94931B18, 0x776AF7F7, 0xAA0DB4AF, 0xAAAAAAAA, 0x18AAAAAA, 0x7A6C98E3, 0xBC99A69A, 0x4DA69AF4,
					0xA69AF7BD, 0x89F7BCAB, 0xF7BCE57B, 0xB7AA25CA, 0xEA26BAD4, 0x269BD4D5, 0x33D4ECAA, 0x7A975969,
					0xAAAAAA0D, 0xAAAAAAAA, 0x98E318AA, 0x6AF7F794, 0x0DB4AF77, 0xAAAAAAAA, 0xAAAAAAAA, 0xA548E318,
					0x592B894F, 0xEF4F5525, 0x0D95373E, 0x55555555, 0x55555555, 0x2E4C718C, 0x93E52EF9, 0x5506AAD5,
					0x55555555, 0x8C555555, 0xFAB6AC71, 0x7DB5B4EE, 0xAAAA86AF, 0xAAAAAAAA, 0x38C62AAA, 0x9BFD4526,
					0x54EBAFDB, 0xAAAAAA83, 0xAAAAAAAA, 0x3A38C62A, 0xBB4F7CBB, 0x83753E73, 0xAAAAAAAA, 0x2AAAAAAA,
					0xA52638C6, 0xBAD2B53C, 0x55436DDD, 0x55555555, 0x63155555, 0x3EA4A71C, 0x5CDCBF6D, 0x555541AA,
					0x55555555, 0x1C631555, 0x737EAD93, 0xAEEEB5AE, 0xAAAAAAA1, 0xAAAAAAAA, 0xCA8E318A, 0xEF2E9F3E,
					0x50D4AF6E, 0x55555555, 0xC5555555, 0x9CA4C718, 0xB96A72E7, }; // 92 32-bit words
*/
void bmc_testfill(uint32_t *data_ptr, uint16_t data_count, bmcDecode* bmc_d, pd_frame* pdf, bool debug) {
    for(uint16_t c = 0; c < data_count; c++) {
	bmc_d->inBuf = data_ptr[c];
        bmcProcessSymbols(bmc_d, pdf);
	if(debug) {
	    printf("DBG%u: %X, %X, %X, %X:%X\n", c, bmc_d->inBuf, bmc_d->procBuf, bmc_d->pOffset, bmc_d->procStage, bmc_d->procSubStage);
	}
    }
}
