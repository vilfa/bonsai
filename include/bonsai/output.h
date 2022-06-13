#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

struct bsi_server;

#include "bonsai/desktop/layers.h"
#include "bonsai/desktop/workspace.h"

struct bsi_output
{
    struct bsi_server* server;
    struct wlr_output* output;
    struct timespec last_frame;
    struct wlr_box usable;

    size_t id;              /* Incremental. */
    bool added, destroying; /* If this output has just been added. */

    struct wlr_output_damage* damage;

    struct bsi_workspace* active_workspace;
    struct wl_list workspaces; /* All workspaces that belong to this output. */

    /* Basically an ad-hoc map of linked lists indexable by `enum
     * zwlr_layer_shell_v1_layer`
     * ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND = 0,
     * ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM = 1,
     * ZWLR_LAYER_SHELL_V1_LAYER_TOP = 2,
     * ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY = 3, */
    struct wl_list layers[4]; /* All layers that belong to this output. */

    struct
    {
        /* wlr_output */
        struct wl_listener frame;
        struct wl_listener destroy;
        /* wlr_output_damage */
        struct wl_listener damage_frame;
        /* bsi_workspace */
        struct wl_list workspace; // bsi_workspace_listener::link
    } listen;

    struct wl_list link_server; // bsi_server
};

struct bsi_output*
output_init(struct bsi_output* output,
            struct bsi_server* server,
            struct wlr_output* wlr_output);

void
output_set_usable_box(struct bsi_output* output, struct wlr_box* box);

void
output_surface_damage(struct bsi_output* output,
                      struct wlr_surface* wlr_surface,
                      bool entire_output);

struct bsi_workspace*
output_get_next_workspace(struct bsi_output* output);

struct bsi_workspace*
output_get_prev_workspace(struct bsi_output* output);

void
output_layers_arrange(struct bsi_output* output);

void
output_destroy(struct bsi_output* output);
