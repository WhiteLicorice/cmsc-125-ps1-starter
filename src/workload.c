/*
 * workload.c: yours. Read and validate the workload file.
 *
 * The manual's "The Workload File" section states the grammar and every
 * refusal. ps1_workload_load fills *out with the workers in file order and
 * reports a refusal through *diag.
 */
#include "ps1.h"

ps1_status ps1_workload_load(const char *path, ps1_workload *out, ps1_diag *diag)
{
    /* TODO: Open the workload file for reading.
       On failure, write "ps1: cannot open workload '<path>'" to *diag.
       Return PS1_ERR_REJECTED.
       cases/parse/missing_workload.work
       See "Workload Refusals" in the manual. */

    /* TODO: Read the file one line at a time.
       Refuse a line longer than 511 bytes with "ps1: workload line N is too long".
       cases/parse/long_line.work
       See "The Workload File" in the manual. */

    /* TODO: Cut the line at the first '#'.
       Ignore the text after it.
       Skip a line that is empty after the cut.
       cases/parse/comments_only.work
       See "The Workload File" in the manual. */

    /* TODO: Split the line into fields on spaces and tabs.
       Refuse fewer than 3 fields.
       Refuse more than 4 fields.
       Write "ps1: workload line N: expected 'name arrival burst'" to *diag.
       cases/parse/short_line.work, cases/parse/extra_field.work
       See "Workload Refusals" in the manual. */

    /* TODO: Accept a worker name of 1 to 15 characters.
       Start it with a letter, then letters, digits, '_', or '-'.
       Write "ps1: workload line N: '<name>' is not a worker name".
       cases/parse/bad_name.work, cases/parse/long_name.work
       See "The Workload File" in the manual. */

    /* TODO: Refuse a worker name that an earlier line already holds.
       Write "ps1: workload line N: duplicate worker name '<name>'".
       cases/parse/duplicate_name.work
       See "Workload Refusals" in the manual. */

    /* TODO: Stop at 16 workers.
       Write "ps1: workload line N: more than 16 workers".
       cases/parse/too_many_workers.work
       See "The Workload File" in the manual. */

    /* TODO: Read the arrival as a tick count of 0 or more.
       Write "ps1: workload line N: arrival must be 0 or greater".
       cases/parse/negative_arrival.work, cases/parse/arrival_text.work
       See "Workload Refusals" in the manual. */

    /* TODO: Read the burst as a tick count of 1 or more.
       Write "ps1: workload line N: burst must be at least 1".
       cases/parse/burst_zero.work, cases/parse/burst_text.work
       See "Workload Refusals" in the manual. */

    /* TODO: Refuse an arrival below the arrival on the earlier line.
       Write "ps1: workload line N: arrival A falls below arrival B".
       cases/parse/descending_arrival.work
       See "The Workload File" in the manual. */

    /* TODO: Refuse an arrival beyond the total burst of the earlier lines.
       The work clock can never reach it, so the run would stall.
       Write "ps1: workload line N: arrival A is beyond the B ticks that arrive before it".
       cases/parse/unreachable_arrival.work, cases/parse/arrival_without_zero.work
       See "The Workload File" in the manual. */

    /* TODO: Read the optional fourth field "crash=N", with N from 1 to the burst.
       Refuse a wrong form with "ps1: workload line N: expected 'name arrival burst'".
       Refuse a wrong range with "ps1: workload line N: crash must be from 1 to <burst>".
       cases/parse/crash_zero.work, cases/parse/crash_beyond_burst.work,
       cases/parse/crash_text.work
       See "The Workload File" in the manual. */

    /* TODO: Refuse a workload whose total burst passes 2048 ticks.
       Write "ps1: the work clock would run N ticks, more than 2048".
       cases/parse/total_burst.work, cases/parse/max_burst.work
       See "The Workload File" in the manual. */

    /* TODO: Refuse a workload that holds no workers.
       Write "ps1: workload has no workers".
       cases/parse/empty_workload.work
       See "Workload Refusals" in the manual. */

    /* TODO: Fill *out with the workers in file order on success.
       Set out->count and out->total_ticks.
       Return PS1_OK.
       cases/worked/fifo.work
       See "The Workload File" in the manual. */

    (void)path;
    (void)out;
    snprintf(diag->text, sizeof diag->text, "ps1: workload.c is not implemented");
    return PS1_ERR_REJECTED;
}
