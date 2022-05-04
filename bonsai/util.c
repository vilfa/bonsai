#include <time.h>

#include "bonsai/util.h"

struct timespec
bsi_util_timespec_get()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ts;
}
