#include "cli_commands.h"

void cli_device_info(Cli* cli, std::string& args) {
    cli_printf(cli, "github: https://github.com/Samuelrmlink/rp2040_pd_bmc" EOL);
    const size_t len = 2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1;
    char id[len];
    pico_get_unique_board_id_string(id, len);
    cli_printf(cli, "platform: %s" EOL, FW_PLATFORM);
    cli_printf(cli, "flash_id: %s" EOL, id);
    cli_printf(cli, "fw_build_date: %s" EOL, FW_BUILD_DATE);
    cli_printf(cli, "fw_git_commit: %s" EOL, FW_GIT_COMMIT);
    cli_printf(cli, "fw_git_branch: %s" EOL, FW_GIT_BRANCH);
    cli_printf(cli, "fw_git_tag: %s" EOL, FW_GIT_TAG);
}
