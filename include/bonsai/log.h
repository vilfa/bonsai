#pragma once

#define bsi_log(verb, fmt, ...)                                                \
    _wlr_log(verb,                                                             \
             "\033[1;32m[bonsai]\033[0m"                                       \
             "[%s:%d] " fmt,                                                   \
             __FILE_NAME__,                                                    \
             __LINE__,                                                         \
             ##__VA_ARGS__)

#define bsi_vlog(verb, fmt, args)                                              \
    _wlr_vlog(verb,                                                            \
              "\033[1;32m[bonsai]\033[0m"                                      \
              "[%s:%d] " fmt,                                                  \
              __FILE_NAME__,                                                   \
              __LINE__,                                                        \
              args)

#define bsi_log_errno(verb, fmt, ...)                                          \
    wlr_log(verb,                                                              \
            "\033[1;32m[bonsai]\033[0m" fmt ": %s",                            \
            ##__VA_ARGS__,                                                     \
            strerror(errno))
