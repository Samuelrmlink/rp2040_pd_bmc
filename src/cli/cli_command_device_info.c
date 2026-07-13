#include <pico/unique_id.h>
#include "cli/cli.h"
#include "cli/cli_commands.h"

void cli_device_info(Cli* cli, const char* args) {
    (void)args;
    cli_printf(cli, "git repo: %s\r\n", FW_GIT_REPO);

    char id[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(id, sizeof(id));
    cli_printf(cli, "platform: %s\r\n", FW_PLATFORM);
    cli_printf(cli, "board: %s\r\n", PICO_BOARD);
    cli_printf(cli, "flash_id: %s\r\n", id);
    cli_printf(cli, "fw_build_date: %s\r\n", FW_BUILD_DATE);
    cli_printf(cli, "fw_git_commit: %s\r\n", FW_GIT_COMMIT);
    cli_printf(cli, "fw_git_branch: %s\r\n", FW_GIT_BRANCH);
    cli_printf(cli, "fw_git_tag: %s\r\n", FW_GIT_TAG);
}
