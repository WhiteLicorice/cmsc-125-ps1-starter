/*
 * worker.c: yours. The child side of the fork.
 *
 * A worker waits at birth for one release byte on its standard input. After
 * that it computes freely: one tick of work, one byte on its standard output,
 * and on, without asking the controller for anything. A stop request arrives as
 * SIGTSTP. The worker finishes the tick in its hands, writes a stop byte, and
 * waits on standard input for the next release byte. See "The Controller and
 * Its Workers" in the manual.
 */
#include "ps1.h"

#include <unistd.h>

void ps1_worker_main(const ps1_workload *wl, size_t index)
{
    /* TODO: Install a SIGTSTP handler that sets a stop flag.
       The handler runs between ticks. It only sets a flag, because a signal
       handler may call few functions.
       See "Stopping and Continuing" in the manual. */

    /* TODO: Reset SIGINT to its default action.
       A Ctrl-C in the terminal then ends the worker outright.
       See "The Signal Contract" in the manual. */

    /* TODO: Wait at birth for one release byte on standard input.
       Descriptor 0 is the control pipe. Read with read, and retry when a
       signal interrupts it. No tick runs before the controller releases the
       worker.
       See "Stopping and Continuing" in the manual. */

    /* TODO: At each tick boundary, if the stop flag is set, write the stop
       byte PS1_STOP_BYTE and wait for the next release byte.
       The tick that was running when the request arrived is already reported.
       See "Stopping and Continuing" in the manual. */

    /* TODO: Burn one tick with ps1_tick.
       Use the previous result as the input to each tick.
       Pass that result into the progress byte, so the byte depends on it.
       A seed that reaches no output lets the compiler delete the loop.
       See "One Tick Is One Piece of Work" in the manual. */

    /* TODO: Write one byte on standard output for each finished tick.
       Descriptor 1 is the progress pipe. Write with write, because printf
       buffers the byte when descriptor 1 is a pipe, and it arrives late.
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

    _exit(1);
}
