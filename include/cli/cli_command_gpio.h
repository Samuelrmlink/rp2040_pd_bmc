#ifndef _CLI_COMMAND_GPIO_H
#define _CLI_COMMAND_GPIO_H

static void cli_gpio_list(Cli *cli, char **argv, size_t argc);
static void cli_gpio_out(Cli *cli, char **argv, size_t argc);
static void cli_gpio_in(Cli *cli, char **argv, size_t argc);
static void cli_gpio_i_know(Cli *cli, char **argv, size_t argc);

typedef struct {
    uint8_t pin;
    bool is_led;
    bool danger;
    const char *name;
} GPIOItem;

typedef struct {
    void (*const fn)(Cli *cli, char **argv, size_t argc);
    const char *name;
    const size_t argc;
} CliGPIOCommand;

static const CliGPIOCommand cli_gpio_commands[] = {
    {cli_gpio_list, "list", 0},
    {cli_gpio_out, "out", 2},
    {cli_gpio_in, "in", 1},
    {cli_gpio_i_know, "i_know_what_i'm_doing", 0},
};
static const size_t cli_gpio_commands_count = sizeof(cli_gpio_commands) / sizeof(CliGPIOCommand);

/*
 *  Printed to CLI when 'gpio list' command is run
 */
#ifdef SAMHEDRICK_PICO_PDINTERROGATOR_REV_D_RP2350
static const GPIOItem gpios[] = {
    { .pin = 0,     .name = "UART0_TX", .danger = true},
    { .pin = 1,     .name = "UART0_RX", .danger = true},
    { .pin = 2,     .name = "SPI0_CLK"},
    { .pin = 3,     .name = "SPI0_MOSI"},
    { .pin = 4,     .name = "SPI0_MISO"},
    { .pin = 5,     .name = "CC1_VCONN_EN"/*, .danger = true*/},
    { .pin = 6,     .name = "CC1_RX", .danger = true},
    { .pin = 7,     .name = "CC2_RX", .danger = true},
    { .pin = 8,     .name = "CC2_VCONN_EN"/*, .danger = true*/},
    { .pin = 9,     .name = "CC1_TX_H", .danger = true},
    { .pin = 10,    .name = "CC1_TX_L", .danger = true},
    { .pin = 11,    .name = "CC2_TX_H", .danger = true},
    { .pin = 12,    .name = "CC2_TX_L", .danger = true},
    { .pin = 13,    .name = "PIO_USB_DP", .danger = true},
    { .pin = 14,    .name = "PIO_USB_DM", .danger = true},
    { .pin = 15,    .name = "GP15"},
    { .pin = 16,    .name = "GP16"},
    { .pin = 17,    .name = "GP17"},
    { .pin = 18,    .name = "GP18"},
    { .pin = 19,    .name = "GP19"},
    { .pin = 20,    .name = "SDA", .danger = true},
    { .pin = 21,    .name = "SCL", .danger = true},
    { .pin = 22,    .name = "TCPC_INT", .danger = true},
    { .pin = 23,    .name = "GP23"},
    { .pin = 24,    .name = "GP24"},
    { .pin = 25,    .name = "GP25"},
    { .pin = 26,    .name = "CC1_CONN"/*, .danger = true*/},
    { .pin = 27,    .name = "CC2_CONN"/*, .danger = true*/},
    { .pin = 28,    .name = "GP28_A2"},
};
#else
static const GPIOItem gpios[] = {
    { .pin = 0,     .name = "UART0_TX", .danger = true},
    { .pin = 1,     .name = "UART0_RX", .danger = true},
    { .pin = 2,     .name = "SPI0_CLK"},
    { .pin = 3,     .name = "SPI0_MOSI"},
    { .pin = 4,     .name = "SPI0_MISO"},
    { .pin = 5,     .name = "CC1_VCONN_EN"/*, .danger = true*/},
    { .pin = 6,     .name = "CC1_RX", .danger = true},
    { .pin = 7,     .name = "CC2_RX", .danger = true},
    { .pin = 8,     .name = "CC2_VCONN_EN"/*, .danger = true*/},
    { .pin = 9,     .name = "CC1_TX_H", .danger = true},
    { .pin = 10,    .name = "CC1_TX_L", .danger = true},
    { .pin = 11,    .name = "CC2_TX_H", .danger = true},
    { .pin = 12,    .name = "CC2_TX_L", .danger = true},
    { .pin = 13,    .name = "PIO_USB_DP", .danger = true},
    { .pin = 14,    .name = "PIO_USB_DM", .danger = true},
    { .pin = 15,    .name = "GP15"},
    { .pin = 16,    .name = "GP16"},
    { .pin = 17,    .name = "GP17"},
    { .pin = 18,    .name = "GP18"},
    { .pin = 19,    .name = "GP19"},
    { .pin = 20,    .name = "SDA", .danger = true},
    { .pin = 21,    .name = "SCL", .danger = true},
    { .pin = 22,    .name = "TCPC_INT", .danger = true},
    { .pin = 23,    .name = "GP23"},
    { .pin = 24,    .name = "GP24"},
    { .pin = 25,    .name = "GP25"},
    { .pin = 26,    .name = "CC1_CONN"/*, .danger = true*/},
    { .pin = 27,    .name = "CC2_CONN"/*, .danger = true*/},
    { .pin = 28,    .name = "GP28_A2"},
};
#endif
static const size_t gpios_count = sizeof(gpios) / sizeof(GPIOItem);

#endif
