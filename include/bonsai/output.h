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
 * @brief Holds all outputs the server knows about via signal listeners. The
 * outputs list holds elements of type `struct bsi_output`
 *
 */
struct bsi_outputs
{
    struct bsi_server* bsi_server;

    size_t len;
    struct wl_list outputs;
};

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

    struct bsi_workspaces* bsi_workspaces;
    struct bsi_output_layers* bsi_output_layers; /* wlr_layer_shell_v1 */

    /* Either we listen for all or none, doesn't make sense to keep track of
     * number of listeners. */
    struct
    {
        struct wl_listener frame;
        struct wl_listener damage;
        struct wl_listener needs_frame;
        struct wl_listener precommit;
        struct wl_listener commit;
        struct wl_listener present;
        struct wl_listener bind;
        struct wl_listener enable;
        struct wl_listener mode;
        struct wl_listener description;
        struct wl_listener destroy;
    } listen;

    struct wl_list link;
};

/**
 * @brief Initialize the server outputs struct.
 *
 * @param bsi_outputs Pointer to bsi_outputs struct.
 * @param bsi_server Server owning the outputs.
 * @return struct bsi_outputs* Pointer to initialized struct.
 */
struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs,
                 struct bsi_server* bsi_server);

/**
 * @brief Adds an output to the known server outputs.
 *
 * @param bsi_outputs Pointer to server outputs struct.
 * @param bsi_output Pointer to output.
 */
void
bsi_outputs_add(struct bsi_outputs* bsi_outputs, struct bsi_output* bsi_output);

/**
 * @brief Removes an output from the known server outputs. Make sure to destroy
 * the output.
 *
 * @param bsi_outputs Pointer to server outputs struct.
 * @param bsi_output Pointer to output.
 */
void
bsi_outputs_remove(struct bsi_outputs* bsi_outputs,
                   struct bsi_output* bsi_output);

struct bsi_output*
bsi_outputs_get_active(struct bsi_outputs* bsi_outputs);

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
                struct wlr_output* wlr_output,
                struct bsi_workspaces* bsi_workspaces,
                struct bsi_output_layers* bsi_output_layers);

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
