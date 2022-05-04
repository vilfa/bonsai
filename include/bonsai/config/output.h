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
 * @brief Represents a single output.
 *
 */
struct bsi_output
{
    struct bsi_server* server;

    struct wlr_output* wlr_output;
    struct timespec last_frame;

    struct wl_listener destroy_listener;
    struct wl_listener frame_listener;

    struct wl_list link;
};

struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs);

void
bsi_outputs_add(struct bsi_outputs* bsi_outputs, struct bsi_output* bsi_output);

void
bsi_outputs_remove(struct bsi_outputs* bsi_outputs,
                   struct bsi_output* bsi_output);

size_t
bsi_outputs_len(struct bsi_outputs* bsi_outputs);

void
bsi_output_add_destroy_listener(struct bsi_output* bsi_output,
                                wl_notify_func_t func);

void
bsi_output_add_frame_listener(struct bsi_output* bsi_output,
                              wl_notify_func_t func);
