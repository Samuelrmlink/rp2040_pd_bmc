#include "cli_commands.h"
#include "../pdb_msg.h"
#include "../bmc_rx.h"

typedef struct {
    void (*const fn)(Cli *cli, std::vector<std::string>& argv);
    const char *name;
} CliUsbpdSubcommand;
typedef struct {
    void (*const cb)(Cli *cli);
    const char *option;
} CliUsbpdShowOption;

static bool str_to_int(const char* str, uint8_t* value) {
    char* end;
    long int val = strtol(str, &end, 10);
    if(val > UINT8_MAX || val < 0 || *end != '\0') {
        return false;
    }

    *value = (uint8_t)val;
    return true;
}

void cli_usbpd_show_pdo(Cli *cli) {
    uint32_t test = 0xffAAFFEE;
    cli_printf(cli, "Show PDO here...%X" EOL, test);
}
void cli_usbpd_show_debugstate(Cli *cli) {
    extern bmcRx *pdq_rx;
    cli_printf(cli, "TESTing" EOL);
    cli_printf(cli, "bmcRx objOffset=%u byteOffset=%u upperSymbol=%u inputRollover=%u scrapBits=%X afterScrapOffset=%u inputOffset=%u overflowCount=%u lastOverflow=%u" EOL, pdq_rx->objOffset, pdq_rx->byteOffset, pdq_rx->upperSymbol, pdq_rx->inputRollover, pdq_rx->scrapBits, pdq_rx->afterScrapOffset, pdq_rx->inputOffset, pdq_rx->overflowCount, pdq_rx->lastOverflow);
}
void cli_usbpd_show_overflow(Cli *cli) {
    extern bmcRx *pdq_rx;
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[pdq_rx->lastOverflow].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
}
void cli_usbpd_show_lastframe(Cli *cli) {
    extern bmcRx *pdq_rx;
    for(int i = 0; i < 56; i++) {
        cli_printf(cli, "%02X", (pdq_rx->pdfPtr)[pdq_rx->objOffset - 1].raw_bytes[i]);
    }
    cli_printf(cli, "\n");
}
/*
void cli_usbpd_show_frame(Cli *cli, std::vector<std::string>& arg) {
    uint8_t framenum;
    if(!str_to_int(arg.c_str(), &framenum)) {
        cli_printf(cli, "Invalid frame #: %s" EOL, arg.c_str());
        return;
    }
    cli_printf(cli, "# %u" EOL, framenum);
}
*/


// This function is part of the 'show' subcommand - so while it does indeed 
// 'show help' - it only does so for the 'show' subcommand of the 'usbpd' command
void cli_usbpd_show_help(Cli *cli) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\tpdo\t- Prints the last set of [Source] Power-Data-Objects (PDOs)" EOL);
    cli_printf(cli, "\tdbg\t- Prints some debug information" EOL);
    cli_printf(cli, "\tovf\t- Prints some overflow debug information" EOL);
    cli_printf(cli, "\tlf\t- Prints the last frame debug information" EOL);
//    cli_printf(cli, "\tframe #\t- Prints the frame debug information" EOL);
}
static const CliUsbpdShowOption cli_usbpd_show_options[] = {
    {.cb = cli_usbpd_show_pdo, .option = "pdo"},
    {.cb = cli_usbpd_show_debugstate, .option = "dbg"},
    {.cb = cli_usbpd_show_overflow, .option = "ovf"},
    {.cb = cli_usbpd_show_lastframe, .option = "lf"},
//    {.cb = cli_usbpd_show_frame, .option = "frame"},
    {.cb = cli_usbpd_show_help, .option = "-h"},
    {.cb = cli_usbpd_show_help, .option = "--help"},
};
static const size_t cli_usbpd_show_options_count = 
    sizeof(cli_usbpd_show_options) / sizeof(cli_usbpd_show_options[0]);
void cli_usbpd_show(Cli *cli, std::vector<std::string>& argv) {
    if(argv.size() < 2) {
	// Print help information
	cli_usbpd_show_help(cli);
	return;
    }
    //cli_printf(cli, "TEST-remove: %u" EOL, argv.size());

    for(size_t i = 0; i < cli_usbpd_show_options_count; i++) {
	    const CliUsbpdShowOption *cl = &cli_usbpd_show_options[i];
        if(cl->option == argv[1]) {
            cl->cb(cli);
        }
    }
    return;
}

void cli_usbpd_help(Cli *cli) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\tusbpd show\t- Print state-machine parameters" EOL);
}
static const CliUsbpdSubcommand cli_usbpd_subcommands[] = {
    {.fn = cli_usbpd_show, .name = "show"},
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
