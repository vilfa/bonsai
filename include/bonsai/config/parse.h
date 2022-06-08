#pragma once

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-util.h>

#include "bonsai/config/def.h"

struct bsi_config*
bsi_config_init(struct bsi_config* config, struct bsi_server* server);

void
bsi_config_find(struct bsi_config* config);

void
bsi_config_parse(struct bsi_config* config);

void
bsi_config_apply(struct bsi_config* config);
