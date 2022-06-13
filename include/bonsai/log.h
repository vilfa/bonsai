#pragma once

#include <wlr/util/log.h>

#ifndef __FILE_NAME__
#define __FILE_NAME__ __FILE__
#endif

#define debug(fmt, ...)                                                        \
    _wlr_log(3,                                                                \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define info(fmt, ...)                                                         \
    _wlr_log(2,                                                                \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define error(fmt, ...)                                                        \
    _wlr_log(1,                                                                \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define errn(fmt, ...)                                                         \
    _wlr_log(1,                                                                \
             "[bonsai]"                                                        \
             "[%s:%d] " fmt ": %s",                                            \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__,                                                    \
             strerror(errno));
