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
#include "bonsai/server.h"
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
    free(atom->cmd);
    free(atom);
}

void
config_input_destroy(struct bsi_input_config* conf)
{
    if (conf->kbd_layout)
        free(conf->kbd_layout);
    if (conf->kbd_layout_toggle)
        free(conf->kbd_layout_toggle);
    if (conf->kbd_model)
        free(conf->kbd_model);
    free(conf->devname);
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
            debug("Matched output %s (%s %s)",
                  output->output->name,
                  output->output->make,
                  output->output->model);
            found = true;

            wlr_output_set_custom_mode(output->output, width, height, refresh);
            wlr_output_enable(output->output, true);
            if (!wlr_output_commit(output->output)) {
                error("Failed to commit on output '%s'", output->output->name);
                return false;
            }

            info("Set mode { width=%d, height=%d, refresh=%d } for output "
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
            debug("Found output %s (%s %s), doesn't match current config entry",
                  output->output->name,
                  output->output->make,
                  output->output->model);
        }
    }

    if (!found)
        info("No output matching config for entry '%s'", output_name);

    return false;

error:
    if (cmd)
        util_split_free(&cmd);
    if (resl)
        util_split_free(&resl);
    error("Invalid output config syntax '%s', syntax is 'output <name> "
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
     *  input pointer <name> scroll_natural <yes/no>
     *  input pointer <name> tap <yes/no>
     *  input keyboard <name> layout <layout_spec[,layout_spec[,...]]>
     *  input keyboard <name> layout_toggle <toggle_spec>
     *  input keyboard <name> model <model_spec>
     *  input keyboard <name> repeat_info <n,n>
     */

    static const char* keys[] = {
        [BSI_CONFIG_INPUT_POINTER_ACCEL_SPEED] = "accel_speed",
        [BSI_CONFIG_INPUT_POINTER_ACCEL_PROFILE] = "accel_profile",
        [BSI_CONFIG_INPUT_POINTER_SCROLL_NATURAL] = "scroll_natural",
        [BSI_CONFIG_INPUT_POINTER_TAP] = "tap",
        [BSI_CONFIG_INPUT_KEYBOARD_LAYOUT] = "layout",
        [BSI_CONFIG_INPUT_KEYBOARD_LAYOUT_TOGGLE] = "layout_toggle",
        [BSI_CONFIG_INPUT_KEYBOARD_REPEAT_INFO] = "repeat_info",
        [BSI_CONFIG_INPUT_KEYBOARD_MODEL] = "model",
    };

    char** cmd = NULL;
    size_t len_cmd = util_split_delim(atom->cmd, " ", &cmd, false);
    struct bsi_input_config* config = NULL;

    if (len_cmd != 6) {
        goto error;
    }

    char* conf_spec = cmd[0];
    char* conf_type = cmd[1];
    char* conf_dev = cmd[2];
    char* conf_param = cmd[3];
    char* conf_val = cmd[4];

    if (strcasecmp("input", conf_spec) != 0) {
        goto error;
    }

    /* 0 = pointer, 1 = keyboard */
    int32_t mode = (strcasecmp("pointer", conf_type) == 0)    ? 0
                   : (strcasecmp("keyboard", conf_type) == 0) ? 1
                                                              : -1;

    if (mode < 0) {
        goto error;
    }

    config = calloc(1, sizeof(struct bsi_input_config));

    if (mode == 0) {
        for (size_t i = BSI_CONFIG_INPUT_POINTER_BEGIN + 1;
             i < BSI_CONFIG_INPUT_POINTER_END;
             ++i) {
            if (strcasecmp(keys[i], conf_param) != 0) {
                continue;
            }
            util_strip_quotes(conf_dev);
            config->devname = strdup(conf_dev);
            config->type = i;
            switch (config->type) {
                case BSI_CONFIG_INPUT_POINTER_ACCEL_SPEED: {
                    errno = 0;
                    config->ptr_accel_speed = strtod(conf_val, NULL);
                    if (errno) {
                        goto error;
                    }
                    info("Pointer accel_speed is %.2lf for device '%s'",
                         config->ptr_accel_speed,
                         config->devname);
                    break;
                }
                case BSI_CONFIG_INPUT_POINTER_ACCEL_PROFILE: {
                    if (strcasecmp("none", conf_val) == 0) {
                        config->ptr_accel_profile =
                            LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;
                    } else if (strcasecmp("flat", conf_val) == 0) {
                        config->ptr_accel_profile =
                            LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT;
                    } else if (strcasecmp("adaptive", conf_val) == 0) {
                        config->ptr_accel_profile =
                            LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
                    } else {
                        config->ptr_accel_profile =
                            LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
                    }
                    info("Pointer accel_profile is %d for device '%s'",
                         config->ptr_accel_profile,
                         config->devname);
                    break;
                }
                case BSI_CONFIG_INPUT_POINTER_SCROLL_NATURAL: {
                    config->ptr_natural_scroll =
                        (strcasecmp("yes", conf_val) == 0) ? true : false;
                    info("Pointer natural_scroll is %d for device '%s'",
                         config->ptr_natural_scroll,
                         config->devname);
                    break;
                }
                case BSI_CONFIG_INPUT_POINTER_TAP: {
                    config->ptr_tap =
                        (strcasecmp("yes", conf_val) == 0) ? true : false;
                    info("Pointer tap is %d for device '%s'",
                         config->ptr_tap,
                         config->devname);
                    break;
                }
                default:
                    break;
            }
        }
    } else if (mode == 1) {
        for (size_t i = BSI_CONFIG_INPUT_KEYBOARD_BEGIN + 1;
             i < BSI_CONFIG_INPUT_KEYBOARD_END;
             ++i) {
            if (strcasecmp(keys[i], conf_param) != 0) {
                continue;
            }
            util_strip_quotes(conf_dev);
            config->devname = strdup(conf_dev);
            config->type = i;
            switch (config->type) {
                case BSI_CONFIG_INPUT_KEYBOARD_LAYOUT: {
                    config->kbd_layout = strdup(conf_val);
                    info("Added keyboard layouts '%s' for device '%s'",
                         config->devname,
                         config->kbd_layout);
                    break;
                }
                case BSI_CONFIG_INPUT_KEYBOARD_LAYOUT_TOGGLE: {
                    config->kbd_layout_toggle = strdup(conf_val);
                    info("Keyboard layout toggle for device '%s' is '%s'",
                         config->devname,
                         config->kbd_layout_toggle);
                    break;
                }
                case BSI_CONFIG_INPUT_KEYBOARD_REPEAT_INFO: {
                    char** rinfo = NULL;
                    size_t len_rinfo =
                        util_split_delim(conf_val, ",", &rinfo, true);
                    if (len_rinfo != 3) {
                        util_split_free(&rinfo);
                        goto error;
                    }
                    errno = 0;
                    config->kbd_repeat_rate = strtoul(rinfo[0], NULL, 10);
                    if (errno) {
                        util_split_free(&rinfo);
                        goto error;
                    }
                    errno = 0;
                    config->kbd_repeat_delay = strtoul(rinfo[1], NULL, 10);
                    if (errno) {
                        util_split_free(&rinfo);
                        goto error;
                    }
                    util_split_free(&rinfo);
                    info("Keyboard repeat info for device '%s' is { rate=%d, "
                         "delay=%d }",
                         config->devname,
                         config->kbd_repeat_rate,
                         config->kbd_repeat_delay);
                    break;
                }
                case BSI_CONFIG_INPUT_KEYBOARD_MODEL: {
                    config->kbd_model = strdup(conf_val);
                    info("Keyboard model for device '%s' is '%s'",
                         config->devname,
                         config->kbd_model);
                    break;
                }
                default:
                    break;
            }
        }
    }

    wl_list_insert(&server->config.input, &config->link);

    return true;

error:
    if (config && config->devname)
        free(config->devname);
    if (config)
        free(config);
    util_split_free(&cmd);
    error("Invalid input config syntax, should be: \n"
          "\tinput pointer <name> accel_speed <f.f>\n"
          "\tinput pointer <name> accel_profile <none|flat|adaptive>\n"
          "\tinput pointer <name> scroll_natural <yes/no>\n"
          "\tinput pointer <name> tap <yes/no>\n"
          "\tinput keyboard <name> layout <layout_spec[,layout_spec[,...]]>\n"
          "\tinput keyboard <name> layout_toggle <toggle_spec>\n"
          "\tinput keyboard <name> model <model_spec>\n"
          "\tinput keyboard <name> repeat_info <n,n>\n");
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

    server->config.workspaces = workspaces_max;
    util_split_free(&cmd);

    info("Workspace count is %ld", server->config.workspaces);

    return true;

error:
    util_split_free(&cmd);
    error("Invalid workspace limit syntax '%s', syntax is 'workspace "
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
        error("Invalid wallpaper config syntax '%s', syntax is 'wallpaper "
              "<abs_path>'",
              atom->cmd);
        return false;
    }

    server->config.wallpaper = cmd[1];
    util_split_free(&cmd);

    info("Wallpaper is '%s'", server->config.wallpaper);

    return true;
}
