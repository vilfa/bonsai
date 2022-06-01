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

    size_t id; /* Incremental. */
    bool new;  /* If this output has just been added. */

    struct wlr_output_damage* damage;

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
        /* bsi_workspace */
        struct wl_listener workspace_active;
    } listen;

    struct wl_list link_server; // bsi_server
};

enum bsi_output_extern_prog
{
    BSI_OUTPUT_EXTERN_PROG_WALLPAPER,
    BSI_OUTPUT_EXTERN_PROG_MAX,
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static char* bsi_output_extern_progs[] = { [BSI_OUTPUT_EXTERN_PROG_WALLPAPER] =
                                               "/usr/bin/swaybg" };

static char* bsi_output_extern_progs_args[] = {
    [BSI_OUTPUT_EXTERN_PROG_WALLPAPER] = "--image=assets/Wallpaper-Default.jpg",
};
#pragma GCC diagnostic pop

/**
 * @brief Gets the bsi_output that contains the wlr_output
 *
 * @param server The server.
 * @param wlr_output The output.
 * @return struct bsi_output* The output container.
 */
struct bsi_output*
bsi_outputs_find(struct bsi_server* server, struct wlr_output* wlr_output);

struct bsi_output*
bsi_output_init(struct bsi_output* output,
                struct bsi_server* server,
                struct wlr_output* wlr_output);

void
bsi_output_setup_extern_progs(struct bsi_output* output);

void
bsi_output_surface_damage(struct bsi_output* output,
                          struct wlr_surface* wlr_surface,
                          bool entire_output);

void
bsi_output_destroy(struct bsi_output* output);
