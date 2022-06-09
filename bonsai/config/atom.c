#define _POSIX_C_SOURCE 200809L
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-util.h>
#include <wlr/types/wlr_output.h>

#include "bonsai/config/atom.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/util.h"

struct bsi_config_atom*
bsi_config_atom_init(struct bsi_config_atom* atom,
                     enum bsi_config_atom_type type,
                     const struct bsi_config_atom_impl* impl,
                     const char* config)
{
    atom->type = type;
    atom->impl = impl;
    atom->cmd = strdup(config);
    return atom;
}

void
bsi_config_atom_destroy(struct bsi_config_atom* atom)
{
    free((void*)atom->cmd);
    free(atom);
}

bool
bsi_config_output_apply(struct bsi_config_atom* atom, struct bsi_server* server)
{
    /* Syntax: output <name> mode <w>x<h> refresh <r> */

    char **cmd = NULL, **resl = NULL;
    size_t len_cmd = bsi_util_split_delim((char*)atom->cmd, " ", &cmd);

    if (len_cmd != 7 || strncmp("output", cmd[0], 6) ||
        strncmp("mode", cmd[2], 6) || strncmp("refresh", cmd[4], 7))
        goto error;

    char* output_name = cmd[1];
    char* output_resl = cmd[3];
    char* output_refr = cmd[5];

    size_t len_resl = bsi_util_split_delim(output_resl, "x", &resl);

    if (len_resl != 3)
        goto error;

    errno = 0;
    int32_t ow = strtol(resl[0], NULL, 10);
    if (errno)
        goto error;

    errno = 0;
    int32_t oh = strtol(resl[1], NULL, 10);
    if (errno)
        goto error;

    errno = 0;
    int32_t or = strtol(output_refr, NULL, 10);
    if (errno)
        goto error;

    // struct wlr_output_mode mode = {
    // .width = ow, .height = oh, .refresh = or, .preferred = true
    // };

    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        if (strcmp(output->output->name, output_name) == 0) {
            wlr_output_set_custom_mode(output->output, ow, oh, or);
            // wlr_output_set_mode(output->output, &mode);
            wlr_output_enable(output->output, true);
            if (!wlr_output_commit(output->output)) {
                bsi_error("Failed to commit on output '%s'",
                          output->output->name);
                return false;
            }

            bsi_info("Set mode { width=%d, height=%d, refresh=%d } for output "
                     "%ld/%s",
                     ow,
                     oh,
                     or
                     , output->id, output->output->name);

            return true;
        }
    }

    return false;

error:
    if (cmd)
        bsi_util_split_free(&cmd);
    if (resl)
        bsi_util_split_free(&resl);
    bsi_error("Invalid output config syntax '%s', syntax is 'output <name> "
              "mode <w>x<h> refresh <r>'",
              atom->cmd);
    return false;
}

bool
bsi_config_input_apply(struct bsi_config_atom* atom, struct bsi_server* server)
{
    // TODO: Which kinds of configs do we even want to have?
    return true;
}

bool
bsi_config_workspace_apply(struct bsi_config_atom* atom,
                           struct bsi_server* server)
{
    /* Syntax: workspace count max <n> */
    char** cmd = NULL;
    size_t len_cmd = bsi_util_split_delim((char*)atom->cmd, " ", &cmd);
    if (len_cmd != 5 || strncmp("workspace", cmd[0], 9) ||
        strncmp("count", cmd[1], 5) || strncmp("max", cmd[2], 3)) {
        goto error;
    }

    errno = 0;
    int32_t workspaces_max = strtol(cmd[3], NULL, 10);
    if (errno)
        goto error;

    server->config.workspaces_max = workspaces_max;
    bsi_util_split_free(&cmd);

    bsi_info("Max workspace count is %ld", server->config.workspaces_max);

    return true;

error:
    bsi_util_split_free(&cmd);
    bsi_error("Invalid workspace limit syntax '%s', syntax is 'workspace "
              "count max <n>'",
              atom->cmd);
    return false;
}

bool
bsi_config_wallpaper_apply(struct bsi_config_atom* atom,
                           struct bsi_server* server)
{
    /* Syntax: wallpaper <path> */
    char** cmd = NULL;
    size_t len_cmd = bsi_util_split_delim((char*)atom->cmd, " ", &cmd);
    if (len_cmd != 3) {
        bsi_util_split_free(&cmd);
        bsi_error("Invalid wallpaper config syntax '%s', syntax is 'wallpaper "
                  "<path>'",
                  atom->cmd);
        return false;
    }

    server->config.wallpaper = cmd[1];
    bsi_util_split_free(&cmd);

    bsi_info("Wallpaper path is '%s'", server->config.wallpaper);

    return true;
}
