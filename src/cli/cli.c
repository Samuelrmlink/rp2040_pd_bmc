#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include "cli/cli.h"
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "mailbox_ipc.h"

// #define HEAP_DEBUG 1

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
        cli_flush(cli);
        cli_printf(cli, "\e[%uD", cli->length - cli->cursor);
        //cli_write_str(cli, "\e["); cli_printf(cli, "%zuD", cli->length - cli->cursor);
    }
}
static void cli_redraw_line(Cli* cli) {
    cli_write_str(cli, "\r\e[2K\e[0G>: ");
    cli_write_str(cli, cli->line);
    size_t back = cli->length - cli->cursor;
    if(back > 0) {
        cli_write_str(cli, "\e[");
        char tmp[30];
        sprintf(tmp, "%zuD", back);
        cli_write_str(cli, tmp);
    }
    cli_flush(cli);
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
                    if(cli->cursor < cli->length) {
                        cli_write_str(cli, "\x1B[C");
                        cli->cursor++;
                        cli_redraw_line(cli);
                    }
                    break;
                case 'D': // Left
                    if(cli->cursor > 0) {
                        cli_write_str(cli, "\e[D");
                        cli->cursor--;
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
    /*
    if(!cli->command_exec)
        cli_write_prompt(cli);
    */
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
void cli_log(uint log_level, const char *fmt, ...) {
    char *buf = pvPortMalloc(LOG_BUF_SIZE);
    ASSERT_MALLOC(buf);
    if(!buf) { return; } // Exit if malloc fails
    // Parse input args
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, LOG_BUF_SIZE, fmt, args);
    va_end(args);
    // Create logging message
    loggingMsg *log_msg = pvPortMalloc(sizeof(loggingMsg));
    ASSERT_MALLOC(log_msg);
    if(!log_msg) { vPortFree(buf); return; }
    log_msg->logLevel = log_level;
    log_msg->string = buf;
    // Create IPC parcel
    mailerLabel parcel;
    parcel.payload_type = LoggingMsg;
    parcel.payload_ptr = log_msg;
    // Attach sender to the outgoing IPC parcel (each FreeRTOS task has its own mailbox QueueHandle_t)
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    if(current_task == tskhdl_usb_cli) { parcel.sender = mailbox_cli; }
    else if(current_task == tskhdl_pd_rxf) { parcel.sender = mailbox_tcpc; }
    else if(current_task == tskhdl_pd_pe) { parcel.sender = mailbox_pe; }
    else { parcel.sender = NULL; }
    // Send to CLI mailbox
    if(xQueueSend(mailbox_cli, &parcel, 0) != pdPASS) {
        // Queue full - drop outgoing data
        vPortFree(log_msg->string);
        vPortFree(log_msg);
    }
}

extern void cli_work(void) {
    Cli cli;
    cli_init(&cli);
    uint32_t timestamp_last = 0;

    while (1) {
        // Check for user input (UART and/or CDC ACM)
        int c = getchar_timeout_us(0);
        //cli_printf(&cli, "%d ", c);
        if(c != -2) {
            cli_process_char(&cli, (uint8_t)c);
        } else {
            // Check mailbox
            mailerLabel parcel;
            if(xQueueReceive(mailbox_cli, &parcel, 0) == pdPASS) {
                if(parcel.payload_type == LoggingMsg) {
                    loggingMsg *log = (loggingMsg*) parcel.payload_ptr;
                    // Identify LOGGING task
                    const char *source_str = "Unknown";
                    if(parcel.sender == mailbox_cli) {          source_str = "CLI: "; }
                    else if(parcel.sender == mailbox_tcpc) {    source_str = "TCPC: "; }
                    else if(parcel.sender == mailbox_pe) {      source_str = "PE: "; }
                    // Handle logging levels - TODO: Gate off lower logging levels
                    const char *level_str = "";
                    const char *escape_seq = "";
                    switch(log->logLevel) {
                        case DEBUG_LOG:     level_str = "[DEBUG] "; escape_seq = "\e[34m"; break;
                        case INFO_LOG:      level_str = "[INFO] "; break;
                        case WARNING_LOG:   level_str = "[WARNING] "; escape_seq = "\e[33m"; break; // Color: Yellow
                        case ERROR_LOG:     level_str = "[ERROR] "; escape_seq = "\e[31m"; break;   // Color: Red
                    }
                    // Write to console
                    cli_write_str(&cli, "\e[2K\e[0G");
                    if(strlen(escape_seq)) { cli_write_str(&cli, escape_seq); } // Write escape sequence
                    cli_write_str(&cli, level_str);
                    cli_write_str(&cli, source_str);
                    cli_write_str(&cli, log->string);
                    if(strlen(escape_seq)) { cli_write_str(&cli, "\e[0m"); }    // Write escape sequence (Reset text color)
                    cli_redraw_line(&cli);
                    vPortFree(log->string);
                    vPortFree(log);
                } else {
                    // Handle unexpected parcel type
                }
            }
#ifdef HEAP_DEBUG
            if(time_us_32() > timestamp_last + 1000000) {
                timestamp_last = time_us_32();
                cli_write_str(&cli, "\e[2K\e[0G");
                cli_flush(&cli);
                //cli_printf(&cli, "Test %u\n", time_us_32() / 1000);
                size_t free_heap     = xPortGetFreeHeapSize();
                size_t min_ever_free = xPortGetMinimumEverFreeHeapSize();
                cli_log(DEBUG_LOG, "FreeRTOS heap - free: %u bytes, minimum ever: %u bytes\n", free_heap, min_ever_free);
                cli_redraw_line(&cli);
            }
#endif
        }
    }
}
