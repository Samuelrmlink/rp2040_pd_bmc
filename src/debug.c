#include "main_i.h"

void debug_pin_toggle(uint8_t pin_num) {
    gpio_set_function(pin_num, GPIO_FUNC_SIO);
    gpio_set_dir(pin_num, true);
    if(gpio_get(pin_num))
        gpio_clr_mask(1 << pin_num); // Drive pin low
    else
        gpio_set_mask(1 << pin_num); // Drive pin high
}
