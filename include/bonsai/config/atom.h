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

enum bsi_input_config_type
{
    BSI_CONFIG_INPUT_POINTER_BEGIN,
    BSI_CONFIG_INPUT_POINTER_ACCEL_SPEED,
    BSI_CONFIG_INPUT_POINTER_ACCEL_PROFILE,
    BSI_CONFIG_INPUT_POINTER_SCROLL_NATURAL,
    BSI_CONFIG_INPUT_POINTER_TAP,
    BSI_CONFIG_INPUT_POINTER_END,

    BSI_CONFIG_INPUT_KEYBOARD_BEGIN,
    BSI_CONFIG_INPUT_KEYBOARD_LAYOUT,
    BSI_CONFIG_INPUT_KEYBOARD_LAYOUT_TOGGLE,
    BSI_CONFIG_INPUT_KEYBOARD_REPEAT_INFO,
    BSI_CONFIG_INPUT_KEYBOARD_MODEL,
    BSI_CONFIG_INPUT_KEYBOARD_END,
};

struct bsi_input_config
{
    enum bsi_input_config_type type;
    char* devname;

    enum libinput_config_accel_profile ptr_accel_profile;
    double ptr_accel_speed;
    bool ptr_natural_scroll;
    bool ptr_tap;

    char* kbd_layout;
    char* kbd_layout_toggle;
    char* kbd_model;
    uint32_t kbd_repeat_rate;
    uint32_t kbd_repeat_delay;

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
config_atom_init(struct bsi_config_atom* atom,
                 enum bsi_config_atom_type type,
                 const struct bsi_config_atom_impl* impl,
                 const char* config);

void
config_atom_destroy(struct bsi_config_atom* atom);

void
config_input_destroy(struct bsi_input_config* conf);

bool
config_output_apply(struct bsi_config_atom* atom, struct bsi_server* server);

bool
config_input_apply(struct bsi_config_atom* atom, struct bsi_server* server);

bool
config_workspace_apply(struct bsi_config_atom* atom, struct bsi_server* server);

bool
config_wallpaper_apply(struct bsi_config_atom* atom, struct bsi_server* server);

static const struct bsi_config_atom_impl output_impl = {
    .apply = config_output_apply,
};

static const struct bsi_config_atom_impl input_impl = {
    .apply = config_input_apply,
};

static const struct bsi_config_atom_impl workspace_impl = {
    .apply = config_workspace_apply,
};

static const struct bsi_config_atom_impl wallpaper_impl = {
    .apply = config_wallpaper_apply,
};
