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
    struct wlr_output_configuration_v1* wlr_output_config;
    struct wlr_output_configuration_head_v1* wlr_output_config_head;
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

/**
 * @brief Adds an output to the known server outputs.
 *
 * @param bsi_server The server.
 * @param bsi_output Pointer to output.
 */
void
bsi_outputs_add(struct bsi_server* bsi_server, struct bsi_output* bsi_output);

/**
 * @brief Gets the bsi_output that contains the wlr_output
 *
 * @param bsi_server The server.
 * @param wlr_output The output.
 * @return struct bsi_output* The output container.
 */
struct bsi_output*
bsi_outputs_find(struct bsi_server* bsi_server, struct wlr_output* wlr_output);

/**
 * @brief Removes an output from the known server outputs. Make sure to destroy
 * the output.
 *
 * @param bsi_server The server.
 * @param bsi_output Pointer to output.
 */
void
bsi_outputs_remove(struct bsi_server* bsi_server,
                   struct bsi_output* bsi_output);

struct bsi_output*
bsi_outputs_get_active(struct bsi_server* bsi_server);

/**
 * @brief Initializes a preallocated bsi_output.
 *
 * @param bsi_output The bsi_output.
 * @param bsi_server The server.
 * @param wlr_output The output data.
 * @return struct bsi_output* Pointer to the initialized struct.
 */
struct bsi_output*
bsi_output_init(struct bsi_output* bsi_output,
                struct bsi_server* bsi_server,
                struct wlr_output* wlr_output);

void
bsi_output_setup_extern_progs(struct bsi_output* bsi_output);

/**
 * @brief Remove all active listeners from the specified `bsi_output`.
 *
 * @param bsi_output The output.
 */
void
bsi_output_finish(struct bsi_output* bsi_output);

/**
 * @brief Destroys (calls `free`) on an output.
 *
 * @param bsi_output The output.
 */
void
bsi_output_destroy(struct bsi_output* bsi_output);
