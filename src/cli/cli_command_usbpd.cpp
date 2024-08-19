#include "cli_commands.h"

typedef struct {
    void (*const fn)(Cli *cli, std::vector<std::string>& argv);
    const char *name;
} CliUsbpdSubcommand;


void cli_usbpd_show(Cli *cli, std::vector<std::string>& argv) {
    cli_printf(cli, "<show some data here>" EOL);
}
void cli_usbpd_help(Cli *cli) {
    cli_printf(cli, "Usage: " EOL);
    cli_printf(cli, "\t usbpd show	- Print state-machine parameters" EOL);
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
