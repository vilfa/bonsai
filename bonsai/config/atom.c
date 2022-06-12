#define _POSIX_C_SOURCE 200809L
#include <libinput.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <wayland-util.h>
#include <wlr/types/wlr_output.h>

#include "bonsai/config/atom.h"
#include "bonsai/log.h"
#include "bonsai/output.h"
#include "bonsai/util.h"

struct bsi_config_atom*
config_atom_init(struct bsi_config_atom* atom,
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
config_atom_destroy(struct bsi_config_atom* atom)
{
    free((void*)atom->cmd);
    free(atom);
}

void
config_input_destroy(struct bsi_config_input* conf)
{
    free(conf->device_name);
    if (conf->layout)
        free(conf->layout);
    free(conf);
}

bool
config_output_apply(struct bsi_config_atom* atom, struct bsi_server* server)
{
    /* Syntax: output <name> mode <w>x<h> refresh <r> */

    char **cmd = NULL, **resl = NULL;
    size_t len_cmd = util_split_delim(atom->cmd, " ", &cmd, false);

    if (len_cmd != 7 || strcasecmp("output", cmd[0]) ||
        strcasecmp("mode", cmd[2]) || strcasecmp("refresh", cmd[4]))
        goto error;

    char* output_name = cmd[1];
    util_strip_quotes(output_name);

    char* output_resl = cmd[3];
    char* output_refr = cmd[5];

    size_t len_resl = util_split_delim(output_resl, "x", &resl, false);

    if (len_resl != 3)
        goto error;

    errno = 0;
    int32_t width = strtol(resl[0], NULL, 10);
    if (errno)
        goto error;

    errno = 0;
    int32_t height = strtol(resl[1], NULL, 10);
    if (errno)
        goto error;

    errno = 0;
    int32_t refresh = strtol(output_refr, NULL, 10);
    if (errno)
        goto error;

    bool found = false;
    struct bsi_output* output;
    wl_list_for_each(output, &server->output.outputs, link_server)
    {
        if (strcmp(output->output->name, output_name) == 0) {
            bsi_debug("Matched output %s (%s %s)",
                      output->output->name,
                      output->output->make,
                      output->output->model);
            found = true;

            wlr_output_set_custom_mode(output->output, width, height, refresh);
            wlr_output_enable(output->output, true);
            if (!wlr_output_commit(output->output)) {
                bsi_error("Failed to commit on output '%s'",
                          output->output->name);
                return false;
            }

            bsi_info("Set mode { width=%d, height=%d, refresh=%d } for output "
                     "%ld/%s (%s %s)",
                     width,
                     height,
                     refresh,
                     output->id,
                     output->output->name,
                     output->output->make,
                     output->output->model);

            return true;
        } else {
            bsi_debug(
                "Found output %s (%s %s), doesn't match current config entry",
                output->output->name,
                output->output->make,
                output->output->model);
        }
    }

    if (!found)
        bsi_errno("No output matching name '%s' found", output_name);

    return false;

error:
    if (cmd)
        util_split_free(&cmd);
    if (resl)
        util_split_free(&resl);
    bsi_error("Invalid output config syntax '%s', syntax is 'output <name> "
              "mode <w>x<h> refresh <r>'",
              atom->cmd);
    return false;
}

bool
config_input_apply(struct bsi_config_atom* atom, struct bsi_server* server)
{
    /* Syntax:
     *  input pointer <name> accel_speed <f.f>
     *  input pointer <name> accel_profile <none|flat|adaptive>
     *  input pointer <name> scroll natural
     *  input keyboard <name> layout <layout>
     *  input keyboard <name> repeat_info <n> <n>
     */

    char** cmd = NULL;
    size_t len_cmd = util_split_delim(atom->cmd, " ", &cmd, false);
    struct bsi_config_input* input_config = NULL;

    if (len_cmd < 3 || strcasecmp("input", cmd[0]))
        goto error;

    input_config = calloc(1, sizeof(struct bsi_config_input));

    if (strcasecmp("pointer", cmd[1]) == 0) {
        if (len_cmd != 6)
            goto error;

        util_strip_quotes(cmd[2]);
        input_config->device_name = strdup(cmd[2]);

        if (strcasecmp("accel_speed", cmd[3]) == 0) {
            input_config->type = BSI_CONFIG_INPUT_POINTER_ACCEL_SPEED;
            errno = 0;
            input_config->accel_speed = strtod(cmd[4], NULL);
            if (errno)
                goto error;
        } else if (strcasecmp("accel_profile", cmd[3]) == 0) {
            input_config->type = BSI_CONFIG_INPUT_POINTER_ACCEL_PROFILE;
            if (strcasecmp("none", cmd[4]) == 0) {
                input_config->accel_profile =
                    LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;
            } else if (strcasecmp("flat", cmd[4]) == 0) {
                input_config->accel_profile =
                    LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
            } else if (strcasecmp("adaptive", cmd[4]) == 0) {
                input_config->accel_profile =
                    LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
            } else {
                input_config->accel_profile =
                    LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
            }
        } else if (strcasecmp("scroll", cmd[3]) == 0 &&
                   strcasecmp("natural", cmd[4]) == 0) {
            input_config->type = BSI_CONFIG_INPUT_POINTER_SCROLL_NATURAL;
            input_config->natural_scroll = true;
        } else {
            goto error;
        }
    } else if (strcasecmp("keyboard", cmd[1]) == 0) {
        if (len_cmd != 6 && len_cmd != 7)
            goto error;

        util_strip_quotes(cmd[2]);
        input_config->device_name = strdup(cmd[2]);

        if (strcasecmp("layout", cmd[3]) == 0) {
            input_config->type = BSI_CONFIG_INPUT_KEYBOARD_LAYOUT;
            input_config->layout = strdup(cmd[4]);
        } else if (strcasecmp("repeat_info", cmd[3]) == 0) {
            input_config->type = BSI_CONFIG_INPUT_KEYBOARD_REPEAT_INFO;
            errno = 0;
            input_config->repeat_rate = strtoul(cmd[4], NULL, 10);
            if (errno)
                goto error;
            errno = 0;
            input_config->delay = strtoul(cmd[5], NULL, 10);
            if (errno)
                goto error;
        } else {
            goto error;
        }
    } else {
        goto error;
    }

    wl_list_insert(&server->config.input, &input_config->link);

    return true;

error:
    if (input_config && input_config->device_name)
        free(input_config->device_name);
    if (input_config)
        free(input_config);
    util_split_free(&cmd);
    bsi_error("Invalid input config syntax '%s', syntax is one of: \n"
              "\tinput pointer <name> accel_speed <f.f>\n"
              "\tinput pointer <name> accel_profile <none|flat|adaptive>\n"
              "\tinput pointer <name> scroll natural\n"
              "\tinput keyboard <name> layout <layout>\n"
              "\tinput keyboard <name> repeat_info <n> <n>",
              atom->cmd);
    return false;
}

bool
config_workspace_apply(struct bsi_config_atom* atom, struct bsi_server* server)
{
    /* Syntax: workspace count max <n> */
    char** cmd = NULL;
    size_t len_cmd = util_split_delim(atom->cmd, " ", &cmd, false);
    if (len_cmd != 5 || strcasecmp("workspace", cmd[0]) ||
        strcasecmp("count", cmd[1]) || strcasecmp("max", cmd[2])) {
        goto error;
    }

    errno = 0;
    int32_t workspaces_max = strtol(cmd[3], NULL, 10);
    if (errno)
        goto error;

    server->config.workspaces_max = workspaces_max;
    util_split_free(&cmd);

    bsi_info("Max workspace count is %ld", server->config.workspaces_max);

    return true;

error:
    util_split_free(&cmd);
    bsi_error("Invalid workspace limit syntax '%s', syntax is 'workspace "
              "count max <n>'",
              atom->cmd);
    return false;
}

bool
config_wallpaper_apply(struct bsi_config_atom* atom, struct bsi_server* server)
{
    /* Syntax: wallpaper <abs_path> */
    char** cmd = NULL;
    size_t len_cmd = util_split_delim(atom->cmd, " ", &cmd, false);
    if (len_cmd != 3) {
        util_split_free(&cmd);
        bsi_error("Invalid wallpaper config syntax '%s', syntax is 'wallpaper "
                  "<abs_path>'",
                  atom->cmd);
        return false;
    }

    server->config.wallpaper = cmd[1];
    util_split_free(&cmd);

    bsi_info("Wallpaper path is '%s'", server->config.wallpaper);

    return true;
}
