#pragma once

#include <wlr/util/log.h>

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define bsi_debug(fmt, ...)                                                    \
    _wlr_log(3, /* "\x1B[38;5;121m[bonsai]\x1B[1;90m" */                       \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define bsi_info(fmt, ...)                                                     \
    _wlr_log(2, /* "\x1B[38;5;121m[bonsai]\x1B[1;34m" */                       \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define bsi_error(fmt, ...)                                                    \
    _wlr_log(1,                                                                \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define bsi_errno(fmt, ...)                                                    \
    _wlr_log(1,                                                                \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt ": %s",                                            \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__,                                                    \
             strerror(errno));
