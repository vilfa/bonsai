#pragma once

#include <stdbool.h>
#include <time.h>
#include <wayland-server-core.h>

struct bsi_server;

struct timespec
bsi_util_timespec_get();

void
bsi_util_slot_connect(struct wl_signal* signal_memb,
                      struct wl_listener* listener_memb,
                      wl_notify_func_t func);

void
bsi_util_slot_disconnect(struct wl_listener* listener_memb);

/**
 * @brief Sets the proper environment and executes an execve call with the
 * specified argp. Takes into account different possible binary locations. As of
 * now, it searches `/usr/bin` and `/usr/local/bin`.
 *
 * @param argp The execve call argp, **with executable name only**.
 * @param len_argp The argp len.
 * @return true Exec was successful.
 * @return false Exec was unsuccessful.
 */
bool
bsi_util_tryexec(char* const* argp, const size_t len_argp);

/**
 * @brief Sets the proper environment and executes an execve call with the
 * specified argp. Always prefer using the `bsi_util_tryexec(...)`. Only use
 * this if you know the executable will be installed in the same location on all
 * machines.
 *
 * @param argp The execve call argp.
 * @param len_argp The argp len.
 * @return true Exec was successful.
 * @return false Exec was unsuccessful.
 */
bool
bsi_util_forkexec(char* const* argp, const size_t len_argp);

/**
 * @brief Splits a string into parts and prepends the element passed as param
 * `first`. Returns an allocated array of pointers to each split part, with a
 * NULL terminating element.
 *
 * @param first The first element.
 * @param in The string to split.
 * @param delim The delimiter to look for.
 * @param out The pointer to the array of strings allocated dynamically (pass
 * `NULL`).
 * @return size_t The size of the array, including the terminating element.
 */
size_t
bsi_util_split_argsp(char* first, char* in, const char* delim, char*** out);

/**
 * @brief Splits a string into parts. Returns an allocated array of pointers to
 * each split part, with a NULL terminating element.
 *
 * @param in The string to split.
 * @param delim The delimiter to look for.
 * @param out The pointer to the array of strings allocated dynamically (pass
 * `NULL`).
 * @return size_t The size of the array, including the terminating element.
 */
size_t
bsi_util_split_delim(char* in, const char* delim, char*** out);

/**
 * @brief Frees the array and only the array containing the pointers to strings.
 *
 * @param out The array from `bsi_util_split_argsp()` or
 * `bsi_util_split_delim()`.
 */
void
bsi_util_split_free(char*** out);
