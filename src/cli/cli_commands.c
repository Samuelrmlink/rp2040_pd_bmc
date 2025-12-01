#include "cli/cli_commands.h"

void cli_device_info(Cli *cli, const char *args);
void cli_help(Cli *cli, const char *args);
void cli_clear(Cli *cli, const char *args);
void cli_gpio(Cli *cli, const char *args);
void cli_usbpd(Cli *cli, const char *args);

const CliItem cli_items[] = {
    { "!",                  "alias for device_info",                        cli_device_info     },
    { "?",                  "alias for help",                               cli_help            },
    { "help",               "show this help",                               cli_help            },
    { "device_info",        "show device info",                             cli_device_info     },
    { "gpio",               "gpio control",                                 cli_gpio            },
//    { "usbpd",              "USB Power Delivery",                           cli_usbpd           },
    { "clear",              "clear the screen",                             cli_clear           },
    { "cls",                "alias for clear",                              cli_clear           },
};
size_t cli_items_count = sizeof(cli_items) / sizeof(CliItem);
