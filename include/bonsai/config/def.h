#pragma once

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

struct bsi_config_atom_impl
{
    bool (*apply)(struct bsi_config_atom* atom, struct bsi_server* server);
};

struct bsi_config_atom
{
    enum bsi_config_atom_type type;
    const struct bsi_config_atom_impl* impl;
    const char* cmd;
    struct wl_list link;
};

struct bsi_config
{
    struct bsi_server* server;
    struct wl_list atoms; // struct bsi_config_atom
    bool found;
    char path[255];
};

struct bsi_config_atom*
bsi_config_atom_init(struct bsi_config_atom* atom,
                     enum bsi_config_atom_type type,
                     const struct bsi_config_atom_impl* impl,
                     const char* config);

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
