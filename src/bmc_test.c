#include "main_i.h"

void bmc_fill_check() {
    extern uint8_t buf1_input_count;
    extern bool buf1_rollover;
    if(buf1_input_count == 255)
	buf1_rollover = true;
    buf1_input_count++;
}
void bmc_fill() { // TODO: remove this
    extern uint32_t* buf1;
    extern uint8_t buf1_input_count;
    buf1[buf1_input_count] = 0xDCDCDCDC;
    bmc_fill_check();
}
void bmc_fill2() {
    extern uint32_t* buf1;
    extern uint8_t buf1_input_count;
    buf1[buf1_input_count] = 0xAAAAA800;//0
    bmc_fill_check();
    buf1[buf1_input_count] = 0xAAAAAAAA;//1
    bmc_fill_check();
    buf1[buf1_input_count] = 0x4C6C62AA;//2
    bmc_fill_check();
    buf1[buf1_input_count] = 0xEF253E97;//3
    bmc_fill_check();
    buf1[buf1_input_count] = 0x2BBDF7A5;//4
    bmc_fill_check();
    buf1[buf1_input_count] = 0x56577D55;//5
    bmc_fill_check();
    buf1[buf1_input_count] = 0x55555435;//6
    bmc_fill_check();
    buf1[buf1_input_count] = 0x55555555;//7
    bmc_fill_check();
	buf1[buf1_input_count] = 0x26363155;//8
    bmc_fill_check();
	buf1[buf1_input_count] = 0xD537E4A9;//9
    bmc_fill_check();
	buf1[buf1_input_count] = 0x1BBEDF4B;//10
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555554;//11
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//12
    bmc_fill_check();
	buf1[buf1_input_count] = 0xABA63631;//13
    bmc_fill_check();
	buf1[buf1_input_count] = 0xD2F292B4;//14
    bmc_fill_check();
	buf1[buf1_input_count] = 0xF7BDDEFB;//15
    bmc_fill_check();
	buf1[buf1_input_count] = 0xBC997BDE;//16
    bmc_fill_check();
	buf1[buf1_input_count] = 0xEF7BDEF7;//17
    bmc_fill_check();
	buf1[buf1_input_count] = 0x7BDEF7BD;//18
    bmc_fill_check();
	buf1[buf1_input_count] = 0x4EF2FDEF;//19
    bmc_fill_check();
	buf1[buf1_input_count] = 0x94DFEF7A;//20
    bmc_fill_check();
	buf1[buf1_input_count] = 0x1BC9A529;//21
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//22
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//23
    bmc_fill_check();
	buf1[buf1_input_count] = 0x94931B18;//24
    bmc_fill_check();
	buf1[buf1_input_count] = 0x776AF7F7;//25
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAA0DB4AF;//26
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//27
    bmc_fill_check();
	buf1[buf1_input_count] = 0x18AAAAAA;//28
    bmc_fill_check();
	buf1[buf1_input_count] = 0x7A6C98E3;//29
    bmc_fill_check();
	buf1[buf1_input_count] = 0xBC99A69A;//30
    bmc_fill_check();
	buf1[buf1_input_count] = 0x4DA69AF4;//31
    bmc_fill_check();
	buf1[buf1_input_count] = 0xA69AF7BD;//32
    bmc_fill_check();
	buf1[buf1_input_count] = 0x89F7BCAB;//33
    bmc_fill_check();
	buf1[buf1_input_count] = 0xF7BCE57B;//34
    bmc_fill_check();
	buf1[buf1_input_count] = 0xB7AA25CA;//35
    bmc_fill_check();
	buf1[buf1_input_count] = 0xEA26BAD4;//36
    bmc_fill_check();
	buf1[buf1_input_count] = 0x269BD4D5;//37
    bmc_fill_check();
	buf1[buf1_input_count] = 0x33D4ECAA;//38
    bmc_fill_check();
	buf1[buf1_input_count] = 0x7A975969;//39
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAA0D;//40
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//41
    bmc_fill_check();
	buf1[buf1_input_count] = 0x98E318AA;//42
    bmc_fill_check();
	buf1[buf1_input_count] = 0x6AF7F794;//43
    bmc_fill_check();
	buf1[buf1_input_count] = 0xDB4AF77;//44
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//45
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//46
    bmc_fill_check();
	buf1[buf1_input_count] = 0xA548E318;//47
    bmc_fill_check();
	buf1[buf1_input_count] = 0x592B894F;//48
    bmc_fill_check();
	buf1[buf1_input_count] = 0xEF4F5525;//49
    bmc_fill_check();
	buf1[buf1_input_count] = 0x0D95373E;//50
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//51
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//52
    bmc_fill_check();
	buf1[buf1_input_count] = 0x2E4C718C;//53
    bmc_fill_check();
	buf1[buf1_input_count] = 0x93E52EF9;//54
    bmc_fill_check();
	buf1[buf1_input_count] = 0x5506AAD5;//55
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//56
    bmc_fill_check();
	buf1[buf1_input_count] = 0x8C555555;//57
    bmc_fill_check();
	buf1[buf1_input_count] = 0xFAB6AC71;//58
    bmc_fill_check();
	buf1[buf1_input_count] = 0x7DB5B4EE;//59
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAA86AF;//60
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//61
    bmc_fill_check();
	buf1[buf1_input_count] = 0x38C62AAA;//62
    bmc_fill_check();
	buf1[buf1_input_count] = 0x9BFD4526;//63
    bmc_fill_check();
	buf1[buf1_input_count] = 0x54EBAFDB;//64
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAA83;//65
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//66
    bmc_fill_check();
	buf1[buf1_input_count] = 0x3A38C62A;//67
    bmc_fill_check();
	buf1[buf1_input_count] = 0xBB4F7CBB;//68
    bmc_fill_check();
	buf1[buf1_input_count] = 0x83753E73;//69
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//70
    bmc_fill_check();
	buf1[buf1_input_count] = 0x2AAAAAAA;//71
    bmc_fill_check();
	buf1[buf1_input_count] = 0xA52638C6;//72
    bmc_fill_check();
	buf1[buf1_input_count] = 0xBAD2B53C;//73
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55436DDD;//74
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//75
    bmc_fill_check();
	buf1[buf1_input_count] = 0x63155555;//76
    bmc_fill_check();
	buf1[buf1_input_count] = 0x3EA4A71C;//77
    bmc_fill_check();
	buf1[buf1_input_count] = 0x5CDCBF6D;//78
    bmc_fill_check();
	buf1[buf1_input_count] = 0x555541AA;//79
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//80
    bmc_fill_check();
	buf1[buf1_input_count] = 0x1C631555;//81
    bmc_fill_check();
	buf1[buf1_input_count] = 0x737EAD93;//82
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAEEEB5AE;//83
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAA1;//84
    bmc_fill_check();
	buf1[buf1_input_count] = 0xAAAAAAAA;//85
    bmc_fill_check();
	buf1[buf1_input_count] = 0xCA8E318A;//86
    bmc_fill_check();
	buf1[buf1_input_count] = 0xEF2E9F3E;//87
    bmc_fill_check();
	buf1[buf1_input_count] = 0x50D4AF6E;//88
    bmc_fill_check();
	buf1[buf1_input_count] = 0x55555555;//89
    bmc_fill_check();
	buf1[buf1_input_count] = 0xC5555555;//90
    bmc_fill_check();
	buf1[buf1_input_count] = 0x9CA4C718;//91
    bmc_fill_check();
	buf1[buf1_input_count] = 0xB96A72E7;//92
    bmc_fill_check();
}



void pd_replay_test1() {
    extern PIO pio;
    uint32_t data_buf[12];
    data_buf[0] = 0xAAAAA800;
    data_buf[1] = 0xAAAAAAAA;
    data_buf[2] = 0x4C6C62AA;
    data_buf[3] = 0xEF253E97;
    data_buf[4] = 0x2BBDF7A5;
    data_buf[5] = 0x56577D55;
    data_buf[6] = 0x55555435;
    data_buf[7] = 0x55555555;
    data_buf[8] = 0x26363155;
    data_buf[8] = 0xD537E4A9;
    data_buf[10] = 0x1BBEDF4B;
    data_buf[11] = 0x55555554;
    printf("pd_replay_test1\n");
    gpio_put(8, 1);
    pio_sm_put_blocking(pio, 0, data_buf[0]);
    pio_sm_put_blocking(pio, 0, data_buf[1]);
    pio_sm_put_blocking(pio, 0, data_buf[2]);
    pio_sm_put_blocking(pio, 0, data_buf[3]);
    pio_sm_set_enabled(pio, 0, true);
    pio_sm_put_blocking(pio, 0, data_buf[4]);
    pio_sm_put_blocking(pio, 0, data_buf[5]);
    pio_sm_put_blocking(pio, 0, data_buf[6]);
    pio_sm_put_blocking(pio, 0, data_buf[7]);
    pio_sm_put_blocking(pio, 0, data_buf[8]);
    pio_sm_put_blocking(pio, 0, data_buf[9]);
    pio_sm_put_blocking(pio, 0, data_buf[10]);
    pio_sm_put_blocking(pio, 0, data_buf[11]);
    printf("pio_sm_get_tx_fifo_level: %X\n", pio_sm_get_tx_fifo_level(pio, 0));
    while(!pio_sm_is_tx_fifo_empty(pio, 0)) sleep_us(5);
    sleep_us(195);
    gpio_put(8, 0);

}
