#include "cli/cli_commands.h"
extern "C" {
#include "usb_pd/pd_frame.h"
//#include "bmc_rx.h"
#include "usb_pd/policy_engine.h"
}
//#include "mcu_registers.h"
#include "cli/cli_hex_convert.h"
#include <string.h>
#include "hardware/dma.h"

typedef struct {
    void (*const fn)(Cli *cli, std::vector<std::string>& argv);
    const char *name;
} CliUsbpdSubcommand;
typedef struct {
    void (*const cb)(Cli *cli, uint32_t *argval);
    const char *option;
    const bool argint;
} CliUsbpdShowOption;

static bool str_to_uint(const char* str, uint32_t* value) {
    char* end;
    long int val = strtol(str, &end, 10);
    if(val > UINT32_MAX || val < 0 || *end != '\0') {
        return false;
    }

    *value = (uint32_t)val;
    return true;
}
static uint32_t endian_swap_u32(const uint32_t input) {
    uint8_t* byte = (uint8_t*)&input;
    return (byte[0] << 24) | (byte[1] << 16) | (byte[2] << 8) | byte[3];
}
static uint32_t endian_swap_u16(const uint16_t input) {
    uint8_t* byte = (uint8_t*)&input;
    return (byte[2] << 8) | byte[3];
}
static bool char_is_valid_hex(const char str) {
    if(str == *"x") {
        // This would likely be the 'x' in the '0x' prefix
        return true;
    } else if(str >= *"0" && str <= *"9") {
        return true;
    } else if(str >= *"A" && str <= *"F") {
        return true;
    } else if(str >= *"a" && str <= *"f") {
        return true;
    } else {
        return false;
    }
}
static uint32_t hex_str_to_uint32(const char* str, bool swap_endian) {
    // Measure the incoming data size
    uint8_t num_nibbles = 0;
    for(int i = 0; i < 12; i++) {
        if(!str[i]) {
            break;
        } else if (str[i] == *"x") {
            num_nibbles = 0;
        } else {
            if(!char_is_valid_hex(str[i])) {
                printf("hex_str_to_uint32(): ASCII symbol '%c' cannot be represented as Hex.\n", str[i]);
                return 0;
            }
            num_nibbles++;
        }
    }
    // Return if incoming data size is invalid
    if(num_nibbles > 2 * sizeof(uint32_t)) {
        printf("hex_str_to_uint32(): Invalid data size [ Expected size: %u bytes (%u bits) ]\n", sizeof(uint32_t), 8 * sizeof(uint32_t));
        return 0;
    }
    // Prepare output data
    uint32_t prep = 0;
    char* null_ptr;
    prep = strtoul(str + 2, &null_ptr, 16);
    // Return data with correct endian
    prep = swap_endian ? endian_swap_u32(prep) : prep;
    return prep;
}
static uint32_t str_to_uint_universal(const char* str) {
    if(str[0] == ASCII_ZERO_SYMBOL_OFFSET && str[1] == ASCII_LOWERCASE_X_SYMBOL_OFFSET) {
        return hex_str_to_uint32(str, false);
    } else {
        uint32_t ret_int;
        str_to_uint(str, &ret_int);
        return ret_int;
    }
}
void cli_usbpd_show_srccap(Cli *cli, uint32_t *unused) {
    cli_printf(cli, "Show srccap - test" EOL);
}
// This function is part of the 'show' subcommand - so while it does indeed 
// 'show help' - it only does so for the 'show' subcommand of the 'usbpd' command
void cli_usbpd_show_help(Cli *cli, uint32_t *argval) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\tsrccap\t- Prints the last [Source Capabilities] message" EOL);
}
static const CliUsbpdShowOption cli_usbpd_show_options[] = {
    {.cb = cli_usbpd_show_srccap, .option = "srccap", .argint = false},
    {.cb = cli_usbpd_show_help, .option = "-h", .argint = false},
    {.cb = cli_usbpd_show_help, .option = "--help", .argint = false},
};
static const size_t cli_usbpd_show_options_count = sizeof(cli_usbpd_show_options) / sizeof(cli_usbpd_show_options[0]);
void cli_usbpd_show(Cli *cli, std::vector<std::string>& argv) {
    if(argv.size() < 2) {
        // Print help information
        cli_usbpd_show_help(cli, NULL);
        return;
    }
    for(size_t i = 0; i < cli_usbpd_show_options_count; i++) {
        const CliUsbpdShowOption *cl = &cli_usbpd_show_options[i];
        if(cl->option == argv[1]) {
            uint32_t cmdarg = 0;
            if(cl->argint) {
                str_to_uint(argv[2].c_str(), &cmdarg);
            }
            cl->cb(cli, &cmdarg);
        }
    }
    return;
}
void cli_usbpd_help(Cli *cli) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\tusbpd show\t- Print state-machine parameters" EOL);
    //cli_printf(cli, "\tusbpd config\t-Configure USB-PD parameters" EOL);
}
static const CliUsbpdSubcommand cli_usbpd_subcommands[] = {
    {.fn = cli_usbpd_show, .name = "show"},
    //{.fn = cli_usbpd_config, .name = "config"},
};
static const size_t cli_usbpd_subcommands_count = 
    sizeof(cli_usbpd_subcommands) / sizeof(cli_usbpd_subcommands[0]);
void cli_usbpd(Cli *cli, std::string& args) {
    std::vector<std::string> argv = cli_split_args(args);
    if(argv.size() < 1) {
        cli_usbpd_help(cli);
        return;
    }

    for(size_t i = 0; i < cli_usbpd_subcommands_count; i++) {
	const CliUsbpdSubcommand *cmd = &cli_usbpd_subcommands[i];
	if(cmd->name == argv[0]) {
	    cmd->fn(cli, argv);
	    return;
	}
    }
    // Show help if no valid subcommand was specified
    cli_usbpd_help(cli);
}
