#pragma once

#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>

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

struct bsi_outputs*
bsi_outputs_init(struct bsi_outputs* bsi_outputs);
