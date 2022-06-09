#pragma once

#include <libinput.h>
#include <stdbool.h>
#include <wayland-util.h>

struct bsi_config_atom;
struct bsi_server;

enum bsi_config_atom_type
{
    BSI_CONFIG_ATOM_OUTPUT,
    BSI_CONFIG_ATOM_INPUT,
    BSI_CONFIG_ATOM_WORKSPACE,
    BSI_CONFIG_ATOM_WALLPAPER,
};

enum bsi_config_input_type
{
    BSI_CONFIG_INPUT_POINTER_ACCEL_SPEED,
    BSI_CONFIG_INPUT_POINTER_ACCEL_PROFILE,
    BSI_CONFIG_INPUT_POINTER_SCROLL_NATURAL,
    BSI_CONFIG_INPUT_KEYBOARD_LAYOUT,
    BSI_CONFIG_INPUT_KEYBOARD_REPEAT_INFO,
};

struct bsi_config_input
{
    enum bsi_config_input_type type;
    char* device_name;
    double accel_speed;
    enum libinput_config_accel_profile accel_profile;
    bool natural_scroll;
    char* layout;
    uint32_t repeat_rate;
    uint32_t delay;
    struct wl_list link;
};

struct bsi_config_atom_impl
{
    bool (*apply)(struct bsi_config_atom* atom, struct bsi_server* server);
};

struct bsi_config_atom
{
    enum bsi_config_atom_type type;
    const struct bsi_config_atom_impl* impl;
    char* cmd;
    struct wl_list link;
};

struct bsi_config_atom*
bsi_config_atom_init(struct bsi_config_atom* atom,
                     enum bsi_config_atom_type type,
                     const struct bsi_config_atom_impl* impl,
                     const char* config);

void
bsi_config_atom_destroy(struct bsi_config_atom* atom);

void
bsi_config_input_destroy(struct bsi_config_input* conf);

bool
bsi_config_output_apply(struct bsi_config_atom* atom,
                        struct bsi_server* server);

bool
bsi_config_input_apply(struct bsi_config_atom* atom, struct bsi_server* server);

bool
bsi_config_workspace_apply(struct bsi_config_atom* atom,
                           struct bsi_server* server);

bool
bsi_config_wallpaper_apply(struct bsi_config_atom* atom,
                           struct bsi_server* server);

static const struct bsi_config_atom_impl output_impl = {
    .apply = bsi_config_output_apply,
};

static const struct bsi_config_atom_impl input_impl = {
    .apply = bsi_config_input_apply,
};

static const struct bsi_config_atom_impl workspace_impl = {
    .apply = bsi_config_workspace_apply,
};

static const struct bsi_config_atom_impl wallpaper_impl = {
    .apply = bsi_config_wallpaper_apply,
};
