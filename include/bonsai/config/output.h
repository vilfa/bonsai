#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

/**
 * @brief Holds all outputs the server knows about via signal listeners. The
 * outputs list holds elements of type `struct bsi_output`
 *
 */
struct bsi_outputs
{
    size_t len;
    struct wl_list outputs;
};

/**
 * @brief Holds all possible active listener types for `bsi_output`.
 *
 */
enum bsi_output_listener_mask
{
    BSI_OUTPUT_LISTENER_FRAME = 1 << 0,
    BSI_OUTPUT_LISTENER_DAMAGE = 1 << 1,
    BSI_OUTPUT_LISTENER_NEEDS_FRAME = 1 << 2,
    BSI_OUTPUT_LISTENER_PRECOMMIT = 1 << 3,
    BSI_OUTPUT_LISTENER_COMMIT = 1 << 4,
    BSI_OUTPUT_LISTENER_PRESENT = 1 << 5,
    BSI_OUTPUT_LISTENER_BIND = 1 << 6,
    BSI_OUTPUT_LISTENER_ENABLE = 1 << 7,
    BSI_OUTPUT_LISTENER_MODE = 1 << 8,
    BSI_OUTPUT_LISTENER_DESCRIPTION = 1 << 9,
    BSI_OUTPUT_LISTENER_DESTROY = 1 << 10,
};

#define bsi_output_listener_len 11

/**
 * @brief Represents a single output and its event listeners.
 *
 */
struct bsi_output
{
    struct bsi_server* bsi_server;
    struct wlr_output* wlr_output;
    struct timespec last_frame;

    uint32_t active_listeners;
    struct wl_list* active_links[bsi_output_listener_len];
    size_t len_active_links;
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
    } events;

    struct wl_list link;
};

/**
 * @brief Initialize the server outputs struct.
 *
 * @param bsi_outputs Pointer to bsi_outputs struct.
 * @return struct bsi_outputs* Pointer to initialized struct.
 */
struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs);

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
 * @brief Destroys (calls `free`) on an output.
 *
 * @param bsi_output The output.
 */
void
bsi_output_destroy(struct bsi_output* bsi_output);

/**
 * @brief Add a listener `func` for the specified member of the `bsi_output`
 * `events` struct.
 *
 * @param bsi_output The output.
 * @param bsi_listener_type Type of listener to add.
 * @param bsi_listener_memb Pointer to a listener to initialize with func (a
 * member of the `events` anonymus struct).
 * @param bsi_signal_memb Pointer to signal which the listener handles (usually
 * a member of the `events` struct of its parent).
 * @param func The listener function.
 */
void
bsi_output_listener_add(struct bsi_output* bsi_output,
                        enum bsi_output_listener_mask bsi_listener_type,
                        struct wl_listener* bsi_listener_memb,
                        struct wl_signal* bsi_signal_memb,
                        wl_notify_func_t func);

/**
 * @brief Remove all active listeners from the specified `bsi_output`.
 *
 * @param bsi_output The output.
 */
void
bsi_output_listeners_unlink_all(struct bsi_output* bsi_output);
