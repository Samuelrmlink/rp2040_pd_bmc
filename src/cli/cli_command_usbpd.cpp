#include "cli_commands.h"
#include "../pd_frame.h"
#include "../bmc_rx.h"
#include "cli_hex_convert.h"

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
static bool hex_str_to_uint8_array(const char* str, uint8_t* value, uint8_t max_bytes_out) {
    uint8_t start_offset = 0;
    uint8_t char_sym;

    // Validate inputs
    if(!str || !value) { return false; }
    // Process each ASCII symbol
    for(int i = 0; i < (max_bytes_out * 2 + 3); i++) { // Two chars represent each hex ASCII byte + '0x' prefix + '\0' string terminator
        char_sym = (uint8_t)str[i];
        // Check for the NULL string terminator
        if(!char_sym) {
            // Zero out the remaining bits
            while((i / 2 - start_offset) < max_bytes_out) {
                value[i / 2 - start_offset] = 0;
                i++;
            }
            return true;
        }
        // Ensure this ASCII symbol corresponds to a valid Hex symbol
        if(char_sym < ASCII_ZERO_SYMBOL_OFFSET || (char_sym > ASCII_9_SYMBOL_OFFSET && char_sym < ASCII_A_SYMBOL_OFFSET)
            || (char_sym > ASCII_F_SYMBOL_OFFSET && (char_sym != ASCII_LOWERCASE_X_SYMBOL_OFFSET))) { return false; }
        // Check for valid '0x' prefix
        if(char_sym == ASCII_LOWERCASE_X_SYMBOL_OFFSET && i == 1 && (uint8_t)str[0] == ASCII_ZERO_SYMBOL_OFFSET) {
            start_offset = 1;
            continue;
        }
        // Make sure we don't exceed max_bytes_out
        if(i / 2 - start_offset>= max_bytes_out) { return false; }
        // Clear output value
        if(!(i % 2)) { value[i / 2 - start_offset] = 0; }
        // Convert ASCII symbol to HEX
        value[i / 2 - start_offset] |= ascii_hex_sym_to_uint_sym[char_sym - ASCII_ZERO_SYMBOL_OFFSET] << !(i % 2) * 4;
    }

    // We still haven't encountered the NULL terminator?
    return false;
}
void cli_usbpd_show_srccap(Cli *cli, uint32_t *argval) {
    extern bmcRx *pdq_rx;
    extern uint8_t srccap_index;
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[srccap_index].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
}
void cli_usbpd_show_debugstate(Cli *cli, uint32_t *argval) {
    extern bmcRx *pdq_rx;
    cli_printf(cli, "bmcRx objOffset=%u byteOffset=%u upperSymbol=%u inputRollover=%u scrapBits=%X afterScrapOffset=%u inputOffset=%u overflowCount=%u lastOverflow=%u" EOL, pdq_rx->objOffset, pdq_rx->byteOffset, pdq_rx->upperSymbol, pdq_rx->inputRollover, pdq_rx->scrapBits, pdq_rx->afterScrapOffset, pdq_rx->inputOffset, pdq_rx->overflowCount, pdq_rx->lastOverflow);
}
void cli_usbpd_show_overflow(Cli *cli, uint32_t *argval) {
    extern bmcRx *pdq_rx;
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[pdq_rx->lastOverflow].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
}
void cli_usbpd_show_lastframe(Cli *cli, uint32_t *argval) {
    extern bmcRx *pdq_rx;
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[pdq_rx->objOffset - 1].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
}
void cli_usbpd_show_firstframe(Cli *cli, uint32_t *argval) {
    extern bmcRx *pdq_rx;
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[0].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
}
void cli_usbpd_show_rawframe(Cli *cli, uint32_t *framenum) {
    extern bmcRx *pdq_rx;
    if(*framenum < pdq_rx->rolloverObj) {
    cli_printf(cli, "# %03u | ", *framenum);
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[*framenum].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
    } else {
        cli_printf(cli, "Error: Invalid frame #: %u" EOL, *framenum);
        cli_printf(cli, "Range: [0 - %u]" EOL, pdq_rx->rolloverObj - 1);
        return;
    }
}


// This function is part of the 'show' subcommand - so while it does indeed 
// 'show help' - it only does so for the 'show' subcommand of the 'usbpd' command
void cli_usbpd_show_help(Cli *cli, uint32_t *argval) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\tpdo\t- Prints the last set of [Source] Power-Data-Objects (PDOs)" EOL);
    cli_printf(cli, "\tdbg\t- Prints some debug information" EOL);
    cli_printf(cli, "\tovf\t- Prints some overflow debug information" EOL);
    cli_printf(cli, "\tlf\t- Prints the last frame debug information" EOL);
    cli_printf(cli, "\tff\t- Prints the first frame debug information" EOL);
    cli_printf(cli, "\trf #\t- Prints the raw frame debug information" EOL);
}
static const CliUsbpdShowOption cli_usbpd_show_options[] = {
    {.cb = cli_usbpd_show_srccap, .option = "srccap", .argint = false},
    {.cb = cli_usbpd_show_debugstate, .option = "dbg", .argint = false},
    {.cb = cli_usbpd_show_overflow, .option = "ovf", .argint = false},
    {.cb = cli_usbpd_show_lastframe, .option = "lf", .argint = false},
    {.cb = cli_usbpd_show_firstframe, .option = "ff", .argint = false},
    {.cb = cli_usbpd_show_rawframe, .option = "rf", .argint = true},
    {.cb = cli_usbpd_show_help, .option = "-h", .argint = false},
    {.cb = cli_usbpd_show_help, .option = "--help", .argint = false},
};
static const size_t cli_usbpd_show_options_count = 
    sizeof(cli_usbpd_show_options) / sizeof(cli_usbpd_show_options[0]);
void cli_usbpd_show(Cli *cli, std::vector<std::string>& argv) {
    if(argv.size() < 2) {
	// Print help information
	cli_usbpd_show_help(cli, NULL);
	return;
    }
    //cli_printf(cli, "TEST-remove: %u" EOL, argv.size());

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

void cli_usbpd_config_help(Cli *cli) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\t usbpd config [WORK IN PROGRESS]");
}
void cli_usbpd_config(Cli *cli, std::vector<std::string>& argv) {
    extern bmcRx *pdq_rx;
    if(argv.size() < 2) {
        // print help information
        return;
    }
    uint32_t test_array_number = 55;
    hex_str_to_uint8_array(argv[1].c_str(), (pdq_rx->pdfPtr)[test_array_number].raw_bytes, 56);
    cli_usbpd_show_rawframe(cli, &test_array_number);
}

void cli_usbpd_help(Cli *cli) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\tusbpd show\t- Print state-machine parameters" EOL);
    cli_printf(cli, "\tusbpd config\t-Configure USB-PD parameters" EOL);
}
static const CliUsbpdSubcommand cli_usbpd_subcommands[] = {
    {.fn = cli_usbpd_show, .name = "show"},
    {.fn = cli_usbpd_config, .name = "config"},
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
