/*
 * trap_cost.c
 *
 * Measure the cost of a plain call against the cost of a trap.
 *
 * plain_target() is a local function. The kernel does not take part.
 * syscall(SYS_getpid) crosses into the kernel and comes back.
 *
 * The numbers change with the machine, so the test checks a property.
 * The trap must cost more per call.
 *
 * This measurement is a user-level illustration. It does not measure the
 * kernel's own timer or scheduler.
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L \
 *      trap_cost.c -o trap_cost
 *
 * Output:
 *   plain_call_ns=<number>
 *   trap_call_ns=<number>
 */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define REPETITIONS 1000000L

static volatile long sink = 0;

static long plain_target(void)
{
    return 0;
}

static double nanoseconds_per_call(long repetitions, int use_trap)
{
    struct timespec start;
    struct timespec end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (long i = 0; i < repetitions; i++) {
        if (use_trap) {
            sink += (long)syscall(SYS_getpid);
        } else {
            sink += plain_target();
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    double elapsed = (double)(end.tv_sec - start.tv_sec) * 1e9
                   + (double)(end.tv_nsec - start.tv_nsec);

    return elapsed / (double)repetitions;
}

int main(void)
{
    (void)nanoseconds_per_call(1000L, 0);
    (void)nanoseconds_per_call(1000L, 1);

    double plain = nanoseconds_per_call(REPETITIONS, 0);
    double trap = nanoseconds_per_call(REPETITIONS, 1);

    printf("plain_call_ns=%.3f\n", plain);
    printf("trap_call_ns=%.3f\n", trap);

    return trap > plain ? 0 : 1;
}
