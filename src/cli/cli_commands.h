#pragma once
#include <string>
#include "cli.h"
#include <pico/unique_id.h>

typedef void (*CliCallback)(Cli* cli, std::string& args);

struct CliItem {
    const char* name;
    const char* desc;
    CliCallback callback;
};

extern const CliItem cli_items[];
extern size_t cli_items_count;
