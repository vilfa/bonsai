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
 * @brief Represents a single output and its event listeners.
 *
 */
struct bsi_output
{
    struct bsi_server* server;
    struct wlr_output* wlr_output;
    struct timespec last_frame;

    struct wl_listener destroy;
    struct wl_listener frame;

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
 * @brief Removes an output from the known server outputs.
 *
 * @param bsi_outputs Pointer to server outputs struct.
 * @param bsi_output Pointer to output.
 */
void
bsi_outputs_remove(struct bsi_outputs* bsi_outputs,
                   struct bsi_output* bsi_output);

/**
 * @brief Gets the size of the known server outputs.
 *
 * @param bsi_outputs Pointer to server outputs struct.
 * @return size_t The number of outputs.
 */
size_t
bsi_outputs_len(struct bsi_outputs* bsi_outputs);

/**
 * @brief Adds a destroy listener to a single server output.
 *
 * @param bsi_output The output.
 * @param func The listener function.
 */
void
bsi_output_add_destroy_listener(struct bsi_output* bsi_output,
                                wl_notify_func_t func);

/**
 * @brief Adds a frame listener to a single server output.
 *
 * @param bsi_output The output.
 * @param func The listener function.
 */
void
bsi_output_add_frame_listener(struct bsi_output* bsi_output,
                              wl_notify_func_t func);
