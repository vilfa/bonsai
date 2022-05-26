#pragma once

#include <stdbool.h>
#include <time.h>

struct bsi_server;

/**
 * @brief Gets the current timestamp.
 *
 * @return struct timespec Timestamp struct.
 */
struct timespec
bsi_util_timespec_get();

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
