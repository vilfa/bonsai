#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-util.h>

#include "bonsai/config/atom.h"

struct bsi_config
{
    struct bsi_server* server;
    struct wl_list atoms; // struct bsi_config_atom
    bool found;
    char path[255];
};

struct bsi_config*
config_init(struct bsi_config* config, struct bsi_server* server);

void
config_destroy(struct bsi_config* config);

void
config_find(struct bsi_config* config);

void
config_parse(struct bsi_config* config);

void
config_apply(struct bsi_config* config);
