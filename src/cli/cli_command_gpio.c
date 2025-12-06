#include <pico/stdlib.h>
#include <pico/time.h>
#include <hardware/gpio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "cli/cli.h"
#include "cli/cli_commands.h"

#define MAX_GPIO_ARGS 4  // Subcommand + up to 2 args + null terminator

typedef struct {
    uint8_t pin;
    bool is_led;
    bool danger;
    const char *name;
} GPIOItem;

static const GPIOItem gpios[] = {
    /*
    { .pin = 0,     .name = "UART0_TX", .danger = true},
    { .pin = 1,     .name = "UART0_RX", .danger = true},
    */
    { .pin = 2,     .name = "SPI0_CLK", .danger = true},
    { .pin = 3,     .name = "SPI0_MOSI", .danger = true},
    { .pin = 4,     .name = "SPI0_MISO", .danger = true},
    /*
    { .pin = 5,     .name = "CC1_VCONN_EN", .danger = true},
    { .pin = 6,     .name = "CC1_RX", .danger = true},
    { .pin = 7,     .name = "CC2_RX", .danger = true},
    { .pin = 8,     .name = "CC2_VCONN_EN", .danger = true},
    { .pin = 9,     .name = "CC1_TX_H", .danger = true},
    { .pin = 10,    .name = "CC1_TX_L", .danger = true},
    { .pin = 11,    .name = "CC2_TX_H", .danger = true},
    { .pin = 12,    .name = "CC2_TX_L", .danger = true},
    */
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
    { .pin = 26,    .name = "CC1_CONN", .danger = true},
    { .pin = 27,    .name = "CC2_CONN", .danger = true},
    { .pin = 28,    .name = "GP28_A2"},
};

static const size_t gpios_count = sizeof(gpios) / sizeof(GPIOItem);
static bool danger = false;

static bool str_to_int(const char *str, uint8_t *value) {
    char *end;
    long int val = strtol(str, &end, 10);
    if(val > UINT8_MAX || val < 0 || *end != '\0') {
        return false;
    }
    *value = (uint8_t)val;
    return true;
}

static const GPIOItem *gpio_get_and_prepare(Cli *cli, const char *arg) {
    uint8_t pin;
    if(!str_to_int(arg, &pin)) {
        cli_printf(cli, "Invalid pin: %s\r\n", arg);
        return NULL;
    }

    const GPIOItem *gpio = NULL;
    for(size_t i = 0; i < gpios_count; i++) {
        if(gpios[i].pin == pin) {
            gpio = &gpios[i];
            break;
        }
    }
    if(!gpio) {
        cli_printf(cli, "Pin not found: %s\r\n", arg);
        return NULL;
    }

    if(gpio->danger && !danger) {
        cli_printf(cli, "Dangerous pin: %s. Use 'gpio i_know_what_i'm_doing' to enable danger mode.\r\n", arg);
        return NULL;
    }

    gpio_init(gpio->pin);
    gpio_set_pulls(gpio->pin, false, false);  // Disable pulls by default

    return gpio;
}

static void gpio_input(Cli *cli, const GPIOItem *gpio) {
    gpio_init(gpio->pin);
    gpio_set_dir(gpio->pin, GPIO_IN);
    gpio_pull_up(gpio->pin);
    //vTaskDelay(pdMS_TO_TICKS(25));
    sleep_ms(25);
    uint8_t value = gpio_get(gpio->pin);
    cli_printf(cli, "Value: %d\n", value);
}

static void gpio_output(Cli *cli, const GPIOItem *gpio, uint8_t value) {
    gpio_init(gpio->pin);
    gpio_set_dir(gpio->pin, GPIO_OUT);
    gpio_put(gpio->pin, value);
    cli_printf(cli, "Value set to: %d\n", value);
}

static void cli_gpio_help(Cli *cli) {
    cli_printf(cli, "Usage: gpio <command> [args]\r\n");
    cli_printf(cli, "Commands:\r\n");
    cli_printf(cli, "  list                   - list available GPIOs\r\n");
    cli_printf(cli, "  out <pin> <0|1>        - set pin to output\r\n");
    cli_printf(cli, "  in <pin>               - read pin input\r\n");
    cli_printf(cli, "  i_know_what_i'm_doing  - enable danger mode\r\n");
}

static void cli_gpio_list(Cli *cli, char **argv, size_t argc) {
    (void)argv;
    (void)argc;
    cli_printf(cli, "Available GPIOs:\r\n");
    for(size_t i = 0; i < gpios_count; i++) {
        const GPIOItem *g = &gpios[i];
        cli_printf(cli, "%2d (%s)", g->pin, g->name);
        if(g->is_led) { cli_write_str(cli, " (LED)"); }
        if(g->danger) { cli_write_str(cli, " (danger)"); }
        cli_write_eol(cli);
        cli_flush(cli);
    }
}

static void cli_gpio_out(Cli *cli, char **argv, size_t argc) {
    (void)argc;
    uint8_t value;
    if(!str_to_int(argv[2], &value)) {
        cli_printf(cli, "Invalid value: %s\r\n", argv[2]);
        return;
    }
    if(value != 0 && value != 1) {
        cli_printf(cli, "Invalid value: %s\r\n", argv[2]);
        return;
    }
    const GPIOItem *gpio = gpio_get_and_prepare(cli, argv[1]);
    if(gpio) {
        gpio_output(cli, gpio, value);
    }
}

static void cli_gpio_in(Cli *cli, char **argv, size_t argc) {
    (void)argc;
    const GPIOItem *gpio = gpio_get_and_prepare(cli, argv[1]);
    if(gpio) {
        gpio_input(cli, gpio);
    }
}

static void cli_gpio_i_know(Cli *cli, char **argv, size_t argc) {
    (void)argv;
    (void)argc;
    danger = true;
    cli_printf(cli, "Danger mode enabled. Board may be bricked if you don't know what you're doing!\r\n");
}

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

void cli_gpio(Cli *cli, const char *args) {
    if(!args || strlen(args) == 0) {
        cli_gpio_help(cli);
        return;
    }

    char args_copy[CLI_MAX_LINE];
    strncpy(args_copy, args, sizeof(args_copy) - 1);
    args_copy[sizeof(args_copy) - 1] = '\0';

    char *argv[MAX_GPIO_ARGS];
    size_t argc = 0;
    char *token = strtok(args_copy, " ");
    while (token && argc < MAX_GPIO_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }

    if(argc == 0) {
        cli_gpio_help(cli);
        return;
    }

    for(size_t i = 0; i < cli_gpio_commands_count; i++) {
        const CliGPIOCommand *cmd = &cli_gpio_commands[i];
        if(strcmp(cmd->name, argv[0]) == 0 && cmd->argc == argc - 1) {
            cmd->fn(cli, argv, argc);
            return;
        }
    }

    cli_gpio_help(cli);
}
