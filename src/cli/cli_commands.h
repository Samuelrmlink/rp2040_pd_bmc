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

typedef void (*CliSubCallback)(const char* str); 
struct CliOption {
    const char* fullname;
    const char* shortname;
    const char* desc;
    CliSubCallback callback;
    const uint8_t num_args;
};

extern const CliItem cli_items[];
extern size_t cli_items_count;
