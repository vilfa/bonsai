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
    struct bsi_server* bsi_server;
    struct wlr_output* wlr_output;
    struct timespec last_frame;
    size_t id; /* Incremental id. */

    struct
    {
        size_t len;
        struct wl_list workspaces;
    } workspace;

    struct
    {
        /* Basically an ad-hoc map indexable by `enum zwlr_layer_shell_v1_layer`
         * ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND = 0,
         * ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM = 1,
         * ZWLR_LAYER_SHELL_V1_LAYER_TOP = 2,
         * ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY = 3, */
        size_t len_layers[4];
        struct wl_list layers[4];
    } layer;

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    struct
    {
        struct wl_listener frame;
        struct wl_listener destroy;
    } listen;

    struct wl_list link;
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
