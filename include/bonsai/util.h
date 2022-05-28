#pragma once

#include <stdbool.h>
#include <time.h>
#include <wayland-server-core.h>

struct bsi_server;

/**
 * @brief Gets the current timestamp.
 *
 * @return struct timespec Timestamp struct.
 */
struct timespec
bsi_util_timespec_get();

/**
 * @brief Connects a signal to its listener and adds a handler.
 *
 * @param bsi_signal_memb The signal.
 * @param bsi_listener_memb The listener.
 * @param func Handler func.
 */
void
bsi_util_slot_connect(struct wl_signal* bsi_signal_memb,
                      struct wl_listener* bsi_listener_memb,
                      wl_notify_func_t func);

/**
 * @brief Disconnects a listener. Is just a nice wrapper around
 * `wl_list_remove`.
 *
 * @param bsi_listener_memb The listener.
 */
void
bsi_util_slot_disconnect(struct wl_listener* bsi_listener_memb);

/**
 * @brief Sets the proper environment and executes an execve call with the
 * specified argp.
 *
 * @param bsi_server The server.
 * @param argp The execve call argp.
 * @param len_argp The argp len.
 * @return true Exec was successful.
 * @return false Exec was unsuccessful.
 */
bool
bsi_util_forkexec(char* const* argp, const size_t len_argp);
