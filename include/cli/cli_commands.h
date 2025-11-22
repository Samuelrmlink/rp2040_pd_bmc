#ifndef _CLI_COMMANDS_H
#define _CLI_COMMANDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "pico/types.h"

#define CLI_MAX_LINE 256
#define CLI_MAX_ARGS 256

typedef struct Cli {
    char line[CLI_MAX_LINE + 1];
    char prev_line[CLI_MAX_LINE + 1];
    size_t cursor;
    size_t length;
    bool esc_key_pressed;
    bool esc_sequence;
    bool command_exec;
} Cli;

typedef void (*CliCallback)(Cli *cli, const char* args);
typedef struct {
    const char *name;
    const char *desc;
    CliCallback callback;
} CliItem;

typedef struct {
    const char *fullname;
    const char *shortname;
    const char *desc;
    void *callback;
    const uint num_args;
} CliOption;

extern const CliItem cli_items[];
extern size_t cli_items_count;

#endif
