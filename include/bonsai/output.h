#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

struct bsi_server;

#include "bonsai/desktop/layer.h"
#include "bonsai/desktop/workspace.h"

/**
 * @brief Represents a single output and its event listeners.
 *
 */
struct bsi_output
{
    struct bsi_server* server;
    struct wlr_output* wlr_output;
    struct timespec last_frame;

    size_t id; /* Incremental id. */
    bool new;  /* If this output has just been added. */

    struct wlr_output_damage* damage;

    struct
    {
        size_t len;
        struct wl_list workspaces;
    } wspace;

    struct
    {
        /* Basically an ad-hoc map indexable by `enum zwlr_layer_shell_v1_layer`
         * ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND = 0,
         * ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM = 1,
         * ZWLR_LAYER_SHELL_V1_LAYER_TOP = 2,
         * ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY = 3, */
        size_t len[4];
        struct wl_list layers[4];
    } layer;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    struct
    {
        /* wlr_output */
        struct wl_listener frame;
        struct wl_listener destroy;
        /* bsi_workspace */
        struct wl_listener workspace_active;
    } listen;

    struct wl_list link;
};

enum bsi_output_extern_prog
{
    BSI_OUTPUT_EXTERN_PROG_WALLPAPER,
    BSI_OUTPUT_EXTERN_PROG_MAX,
};

static char* bsi_output_extern_progs[] = { [BSI_OUTPUT_EXTERN_PROG_WALLPAPER] =
                                               "/usr/bin/swaybg" };

static char* bsi_output_extern_progs_args[] = {
    [BSI_OUTPUT_EXTERN_PROG_WALLPAPER] = "--image=assets/Wallpaper-Default.jpg",
};

void
bsi_outputs_add(struct bsi_server* server, struct bsi_output* output);

/**
 * @brief Gets the bsi_output that contains the wlr_output
 *
 * @param server The server.
 * @param wlr_output The output.
 * @return struct bsi_output* The output container.
 */
struct bsi_output*
bsi_outputs_find(struct bsi_server* server, struct wlr_output* wlr_output);

void
bsi_outputs_remove(struct bsi_server* server, struct bsi_output* output);

struct bsi_output*
bsi_outputs_get_active(struct bsi_server* server);

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
bsi_output_finish(struct bsi_output* output);

void
bsi_output_destroy(struct bsi_output* output);
