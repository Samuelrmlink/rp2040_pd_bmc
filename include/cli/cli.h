#ifndef _CLI_H
#define _CLI_H

#include "cli/cli_commands.h"
#include <stdio.h>

#define LOG_BUF_SIZE 256

void cli_init(Cli *cli);
void cli_process_char(Cli *cli, uint8_t c);

void cli_write_str(Cli *cli, const char *str);
void cli_write_char(Cli *cli, const char c);
void cli_write_eol(Cli *cli);
void cli_write_prompt(Cli *cli);
void cli_flush(Cli *cli);
void cli_printf(Cli *cli, const char *fmt, ...);
void cli_log(uint log_level, const char *fmt, ...);
void cli_work(void);

#endif
