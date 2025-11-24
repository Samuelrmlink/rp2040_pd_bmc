#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "cli/cli.h"

// Trim leading and trailing whitespace
static char* str_trim(char *s) {
    // Locate starting character (trim leading whitespace)
    char *start = s;
    while(isspace((unsigned char) *start)) { start++; }
    // Locate ending character (trim trailing whitespace)
    size_t len = strlen(start);
    while(len > 0 && isspace((unsigned char) start[len - 1])) { len--; }
    // Set NULL char to denote the end of string
    start[len] = '\0';
    return start;
}
// Retrieve CliItem entry by name
static const CliItem* cli_find_command(const char *name) {
    for(size_t i = 0; i < cli_items_count; i++) {
        if(strcmp(cli_items[i].name, name) == 0) { return &cli_items[i]; }
    }
    return NULL;
}
// Handle 'Enter' key on CLI
static void cli_handle_enter(Cli *cli) {
    cli_write_eol(cli);
    char cmd_name[64];
    char *line_trimmed = str_trim(cli->line);
    // Handle no command
    if(line_trimmed[0] == '\0') {
        cli_write_prompt(cli);
        cli_flush(cli);
        return;
    }
    // Extract command name
    size_t cmd_len = 0;
    while(line_trimmed[0] && !isspace((unsigned char) line_trimmed[cmd_len])) { cmd_len++; }
    size_t cmd_name_len = cmd_len < sizeof(cmd_name) - 1 ? cmd_len : sizeof(cmd_name) - 1;
    memcpy(cmd_name, line_trimmed, cmd_name_len);
    cmd_name[cmd_name_len] = '\0';
    // Find args offset
    const char *args = line_trimmed + cmd_len;
    while(isspace((unsigned char) *args)) { args++; }
    // Run command
    const CliItem *item = cli_find_command(cmd_name);
    if(item) {
        cli->command_exec = true;
        item->callback(cli, args);
        cli->command_exec = false;
    } else {
        cli_write_str(cli, "Command not found.");
        cli_write_eol(cli);
    }
    // Save previous command history
    strncpy(cli->prev_line, line_trimmed, CLI_MAX_LINE);
    cli->prev_line[CLI_MAX_LINE] = '\0';
    // Reset current line
    cli->line[0] = '\0';
    cli->length = 0;
    cli->cursor = 0;
    cli_write_eol(cli);
    cli_write_prompt(cli);
    cli_flush(cli);
}
static void cli_handle_backspace(Cli* cli) {
    if(cli->cursor == 0) {
        // Bell sound
        cli_write_char(cli, '\a');
        return;
    }
    memmove(cli->line + cli->cursor - 1,
            cli->line + cli->cursor,
            cli->length - cli->cursor + 1);
    cli->length--;
    cli->cursor--;
    cli_write_str(cli, "\e[D\e[1P");
}
static void cli_insert_char(Cli* cli, char c) {
    if(cli->length >= CLI_MAX_LINE)
        return;

    //printf("cur %u", cli->cursor);
    memmove(cli->line + cli->cursor + 1,
            cli->line + cli->cursor,
            cli->length - cli->cursor + 1);
    cli->line[cli->cursor] = c;
    cli->length++;
    cli->cursor++;

    cli_write_char(cli, c);
    if(cli->cursor < cli->length) {
        cli_write_str(cli, cli->line + cli->cursor);
        cli_write_str(cli, "\e["); cli_printf(cli, "%zuD", cli->length - cli->cursor);
    }
}
static void cli_redraw_line(Cli* cli) {
    cli_write_str(cli, "\r\e[2K\e[0G>: ");
    cli_write_str(cli, cli->line);
    size_t back = cli->length - cli->cursor;
    if(back > 0) {
        cli_write_str(cli, "\e[");
        cli_printf(cli, "%zuD", back);
    }
}
void cli_process_char(Cli* cli, uint8_t c) {
    //printf("cli_process_char: %u %u %u\n", c, cli->esc_key_pressed, cli->esc_sequence);
    // Ensure that we don't process escaped characters
    if(cli->esc_key_pressed == false) {
        switch (c) {
            case 0x1B: // ESC
                cli->esc_key_pressed = true;
                cli->esc_sequence = false;
                break;
            case '\r':
            case '\n':
                cli_handle_enter(cli);
                break;
            case 0x08: // Backspace
            case 0x7F: // DEL
                cli_handle_backspace(cli);
                break;
            case 0x03: // Ctrl+C
                cli->line[0] = '\0';
                cli->length = 0;
                cli->cursor = 0;
                cli_write_eol(cli);
                cli_write_prompt(cli);
                break;
            case 0x04: // Ctrl+D (EOF)
                if(cli->length == 0) {
                    cli_write_str(cli, "^D");
                    cli_write_eol(cli);
                }
                break;
            default:
                if(c >= 32 && c <= 126) {
                    //printf("Insert: %c\n", c);
                    cli_insert_char(cli, c);
                }
                break;
        }
    }
    if(cli->esc_key_pressed) {
        //printf("cli_process_char: %u\n", c);
        if(cli->esc_sequence) {
        //printf("K: %u\n", c);
            switch (c) {
                case 'A': // Up
                    if(cli->length == 0 && cli->prev_line[0] != '\0') {
                        strcpy(cli->line, cli->prev_line);
                        cli->length = strlen(cli->line);
                        cli->cursor = cli->length;
                        cli_redraw_line(cli);
                    }
                    break;
                case 'B': // Down (not implemented - single line history)
                    break;
                case 'C': // Right
                    printf("cursor: %u %u\n", cli->cursor, cli->length);
                    if(cli->cursor < cli->length) {
                        cli_write_str(cli, "\x1B[C");
                        cli->cursor++;
                    }
                    break;
                case 'D': // Left
                    //printf("cursor: %u %u\n", cli->cursor, cli->length);
                    if(cli->cursor > 0) {
                        //cli_write_str(cli, "\x1B[Dleft");
                        cli_write_char(cli, "\e");
                        cli_write_char(cli, "[");
                        cli_write_char(cli, "D");
                        cli->cursor--;
                        //cli->length--;
                    }
                    break;
                default:
                    printf("Invalid Escape [%u]", c);
            }
            cli->esc_key_pressed = false;
            cli->esc_sequence = false;
        } else if(c == '[') {
            cli->esc_sequence = true;
            //return;
        } else if(c == '\e') {
            return;
        } else {
            cli->esc_key_pressed = false;
        }
        //return;
    }
    cli_flush(cli);
}
void cli_write_str(Cli* cli, const char* str) {
    (void)cli;
    fputs(str, stdout);
}
void cli_write_char(Cli* cli, char c) {
    (void)cli;
    putchar(c);
}
void cli_write_eol(Cli* cli) {
    cli_write_str(cli, "\r\n");
}
void cli_write_prompt(Cli* cli) {
    cli_write_str(cli, "\r\e[2K\e[0G>: ");
    cli_write_str(cli, cli->line);
    /*
    if(cli->cursor < cli->length) {
        cli_write_str(cli, "\e[");
        //cli_printf(cli, "%zuD", cli->length - cli->cursor);
    }
    */
}
void cli_flush(Cli* cli) {
    (void)cli;
    fflush(stdout);
}
void cli_printf(Cli* cli, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    if(!cli->command_exec)
        cli_write_prompt(cli);
}
void cli_help(Cli* cli, const char* args) {
    (void)args;
    size_t max_len = 0;
    for(size_t i = 0; i < cli_items_count; i++) {
        size_t len = strlen(cli_items[i].name);
        if(len > max_len) max_len = len;
    }
    max_len += 2;
    for(size_t i = 0; i < cli_items_count; i++) {
        cli_write_str(cli, cli_items[i].name);
        if(cli_items[i].desc) {
            size_t pad = max_len - strlen(cli_items[i].name);
            for(size_t s = 0; s < pad; s++) cli_write_str(cli, " ");
            cli_write_str(cli, "- ");
            cli_write_str(cli, cli_items[i].desc);
        }
        cli_write_eol(cli);
    }
    cli_write_eol(cli);
}
void cli_clear(Cli* cli, const char* args) {
    (void)args;
    cli_write_str(cli, "\e[2J\e[H");
}
void cli_init(Cli* cli) {
    memset(cli, 0, sizeof(Cli));
    cli_write_str(cli, "\r\n=== RP2040 PD BMC CLI ===\r\n");
    cli_write_prompt(cli);
    cli_flush(cli);
}

extern void cli_work(void) {
    Cli cli;
    cli_init(&cli);

    while (1) {
        int c = getchar();
        if(c != EOF)
            cli_process_char(&cli, (uint8_t)c);
    }
}
