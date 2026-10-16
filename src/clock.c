/*
 * clock.c: the given monotonic clock. Do not edit.
 *
 * Every timestamp in this project comes from CLOCK_MONOTONIC, read here. It
 * only moves forward. No clock adjustment can step it backwards, so the
 * difference between two readings is a duration.
 */
#include "ps1.h"

#include <time.h>

double ps1_now(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}
