/*
 * worker.c: yours. The child side of the fork.
 *
 * A worker parks itself at birth. After that it computes freely: one tick of
 * work, one byte on its progress pipe, and on, without asking the controller
 * for anything. See "The Controller and Its Workers" in the manual.
 */
#include "ps1.h"

#include <unistd.h>

void ps1_worker_main(const ps1_workload *wl, size_t index, int progress_fd)
{
    /* TODO: Park the worker at birth with raise(SIGSTOP).
       No tick runs before the controller dispatches the worker.
       See "Stopping and Continuing" in the manual. */

    /* TODO: Reset SIGINT to its default action.
       A Ctrl-C in the terminal then ends the worker outright.
       See "The Signal Contract" in the manual. */

    /* TODO: Burn one tick with ps1_tick.
       Use the previous result as the input to each tick.
       Pass that result into the progress byte, so the byte depends on it.
       A seed that reaches no output lets the compiler delete the loop.
       See "One Tick Is One Piece of Work" in the manual. */

    /* TODO: Write one byte on the progress pipe for each finished tick.
       Retry the write when a signal interrupts it.
       See "One Tick Is One Piece of Work" in the manual. */

    /* TODO: Leave with status 9 right after the crash tick.
       cases/fault/crash_mid_burst.work
       See "The Workload File" in the manual. */

    /* TODO: Leave with status 0 after the last tick.
       Never return to the caller.
       See "The Controller and Its Workers" in the manual. */

    (void)wl;
    (void)index;
    (void)progress_fd;

    _exit(1);
}
