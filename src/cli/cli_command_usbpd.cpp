#include "cli_commands.h"
#include "../pd_frame.h"
#include "../bmc_rx.h"
#include "../policy_engine.h"
#include "../mcu_registers.h"
#include "cli_hex_convert.h"
#include <string.h>

extern const size_t database_items_count = sizeof(database) / sizeof(configKey);


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
    //hex_str_to_uint8_array2(str, (uint8_t*) &prep, 4);
    char* null_ptr;
    prep = strtoul(str + 2, &null_ptr, 16);
    // hex_str_to_uint8_array actually swaps endian - so we have to swap it back if we don't want the endian to be swapped.
    if(swap_endian) {
        return endian_swap_u32(prep);
    } else {
        return prep;
    }
}
static uint16_t hex_str_to_uint16(const char* str, bool swap_endian) {
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
    if(num_nibbles != 2 * sizeof(uint16_t)) {
        printf("hex_str_to_uint32(): Invalid data size [ Expected size: %u bytes (%u bits) ]\n", sizeof(uint16_t), 8 * sizeof(uint16_t));
        return 0;
    }
    // Prepare output data
    uint16_t prep = 0;
    hex_str_to_uint8_array(str, (uint8_t*) &prep, 4);
    // hex_str_to_uint8_array actually swaps endian - so we have to swap it back if we don't want the endian to be swapped.
    if(!swap_endian) {
        return endian_swap_u16(prep);
    } else {
        return prep;
    }
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

void cli_usbpd_config_printkeys() {
    extern configKey* config_db;
    printf("   Keys:          Description:" EOL);
    printf("  -------        --------------" EOL);
    for(int i = 0; i < database_items_count; i++) {
        printf("  %s    %s" EOL, config_db[i].name, config_db[i].desc);
    }
}
void cli_usbpd_config_help(const char* __unused) {
    printf("Usage: usbpd config [OPTIONS]" EOL);
    printf("  -h, --help\t[Shows this message]" EOL);
    printf("  -s, set <Key> <value>" EOL);
    printf("  -g, get <Key>" EOL EOL);
    cli_usbpd_config_printkeys();
}
bool config_key_search(configKey *db, const char* search_key, uint8_t *index) { // Returns true if index is found
    bool db_match = false;
    for(int i = 0; i < database_items_count; i++) {
        if(strcmp(search_key, (db[i].name)) == 0) {
            *index = i;
            db_match = true;
            break;
        }
    }
    return db_match;
}
void cli_usbpd_config_set(const char* key_str, const char* value_str) {
    extern configKey* config_db;
    uint8_t index;
    if(config_key_search(config_db, key_str, &index)) {
        keyData const *key = config_db[index].keyPtr;
        uint32_t input_uint;
        switch(config_db[index].keyDataType) {
            case KeyString:
                // Implement
                printf("KeyString set not implemented\n");
                //key->Ks.ptr_str_ptr = "JJTEST";
                break;
            case KeyBitval:
                input_uint = str_to_uint_universal(value_str);
                // Divide by multiplier (if necessary)
                if(key->Bv.valMultp > 1 & (config_db[index].useMultp)) {
                    input_uint /= key->Bv.valMultp;
                }
                if(input_uint < key->Bv.minValue | input_uint > key->Bv.maxValue) {
                    printf("Error: %u [0x%X] is outside the acceptable register range. {%u, %u} or [0x%X, 0x%X]\n", input_uint, input_uint, key->Bv.minValue, key->Bv.maxValue, key->Bv.minValue, key->Bv.maxValue);
                    return;
                }
                config_reg[key->Bv.regNum] = input_uint << key->Bv.lsbOffset;
                //config_reg[key->Bv.regNum] = 
                printf("Dbg: %u %X\n", input_uint, input_uint);
                break;
            default:
                printf("Error: Invalid key type\n");
        }
    } else {
        printf("Usage: usbpd config get <key>" EOL);
        cli_usbpd_config_printkeys();
    }
    //printf("Config set: %s:%s\n", key_str, value_str);
}
void cli_usbpd_config_get(const char* str, const char* _unused) {
    extern configKey* config_db;
    uint8_t index;
    if(config_key_search(config_db, str, &index)) {
        keyData const *key = config_db[index].keyPtr;
        switch(config_db[index].keyDataType) {
            case KeyString:
                if(*(key->Ks.ptr_str_ptr) == NULL) {
                    printf("NULL STR %s: %s\n", config_db[index].name, key->Ks.default_str);
                } else {
                    printf("%s: %s\n", config_db[index].name, *(key->Ks.ptr_str_ptr));
                }
                break;
            case KeyBitval: {
                uint8_t multiplier;
                if(config_db[index].useMultp) { multiplier = key->Bv.valMultp; } else { multiplier = 1; }
                //printf("Config description: %s, %u %u %u %u\n", config_db[index].desc, key->Bv.regNum, key->Bv.lsbOffset, key->Bv.msbOffset, config_reg[key->Bv.regNum]);
                uint8_t num_bits = key->Bv.msbOffset - key->Bv.lsbOffset + 1;
                uint8_t offset_bits = key->Bv.lsbOffset;
                uint32_t config_val = ((config_reg[key->Bv.regNum] >> offset_bits) & (0xFFFFFFFF >> (32 - num_bits))) * multiplier;
                printf("%s: %u [0x%X]", config_db[index].name, config_val, config_val);
                break;
            }
            default:
                printf("Error: Invalid key type\n");
        }
    } else {
        printf("Usage: usbpd config get <key>" EOL);
        cli_usbpd_config_printkeys();
    }
}
static const CliOption usbpdConfigOptions[] = {
    {
        .fullname = "--help",
        .shortname = "-h",
        .desc = "Shows help",
        .callback = (void*)(uintptr_t)cli_usbpd_config_help,
        .num_args = 0,
    },
    {
        .fullname = "set",
        .shortname = "-s",
        .desc = "usbpd config set <key> <value>",
        .callback = (void*)(uintptr_t)cli_usbpd_config_set,
        .num_args = 2,
    },
    {
        .fullname = "get",
        .shortname = "-g",
        .desc = "usbpd config get <key> <value>",
        .callback = (void*)(uintptr_t)cli_usbpd_config_get,
        .num_args = 1,
    },
};
static const size_t usbpdConfigOptions_count = sizeof(usbpdConfigOptions) / sizeof(usbpdConfigOptions[0]);
void cli_usbpd_config(Cli *cli, std::vector<std::string>& argv) {
    extern bmcRx *pdq_rx;
    if(argv.size() < 2) {
        // print help information
        cli_usbpd_config_help(NULL);
        return;
    }
    for(int i = 1; i < (int)argv.size(); i++) {
        for(size_t j = 0; j < usbpdConfigOptions_count; j++) {
            const CliOption cmd_co = usbpdConfigOptions[j];
            // Check whether the current string matches a valid option
            if((argv[i] == cmd_co.shortname) || (argv[i] == cmd_co.fullname)) {
                // Check that we have enough arguments that whichever option was chosen
                if((argv.size() - 1 - i) >= cmd_co.num_args) {
                    // Execute the appropriate option handler
                    if(!cmd_co.num_args) {
                        void (*func_ptr)(void) = (void (*)(void))cmd_co.callback;
                        func_ptr();
                    } else if(cmd_co.num_args == 1) {
                        void (*func_ptr)(const char*) = (void (*)(const char*))cmd_co.callback;
                        func_ptr(argv[i + 1].c_str());
                    } else if(cmd_co.num_args == 2) {
                        void (*func_ptr)(const char*, const char*) = (void (*)(const char*, const char*))cmd_co.callback;
                        func_ptr(argv[i + 1].c_str(), argv[i + 2].c_str());
                    }
                    // Offset the number of args
                    i += cmd_co.num_args;
                } else {
                    cli_printf(cli, "Error: Option [ %s, %s ] requires at least %u arguments.\n", cmd_co.shortname, cmd_co.fullname, cmd_co.num_args);
                    // Print usbpd config usage tips
                    cli_usbpd_config_get("help", NULL);
                }
            }
        }
    }
/*
    uint32_t test_array_number = 55;
    hex_str_to_uint8_array(argv[1].c_str(), (pdq_rx->pdfPtr)[test_array_number].raw_bytes, 56);
    cli_usbpd_show_rawframe(cli, &test_array_number);
*/
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
