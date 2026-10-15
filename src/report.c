/*
 * report.c: yours. The metrics.
 *
 * The report has one header line, one line per worker in workload order, and
 * one average line. The manual's "The Report" section states every field.
 */
#include "ps1.h"

ps1_status ps1_report_print(const ps1_run *run, FILE *out)
{
    /* TODO: Write the header line.
       Give the policy, the quantum, the worker count, and the run's tick count.
       policy fifo quantum 0 workers 3 ticks 17
       cases/worked/fifo.work
       See "The Report" in the manual. */

    /* TODO: Write one worker line for each worker, in workload order.
       Echo the arrival and the burst from the workload.
       cases/normal/fifo_two.work
       See "The Report" in the manual. */

    /* TODO: Write response_ticks as the first dispatch tick minus the arrival tick.
       cases/normal/fifo_two.work
       See "The Report" in the manual. */

    /* TODO: Write response_ms as that gap in milliseconds, with six decimals.
       See "The Measurement Contract" in the manual. */

    /* TODO: Write turnaround_ticks as the completion tick minus the arrival tick.
       cases/worked/rr.work
       See "The Report" in the manual. */

    /* TODO: Write turnaround_ms as that gap in milliseconds, with six decimals.
       See "The Measurement Contract" in the manual. */

    /* TODO: Write the dispatch count and the preemption count for each worker.
       cases/normal/rr_two.work
       See "The Report" in the manual. */

    /* TODO: Write a single dash for a metric the run never produced.
       A worker that never ran has no response time.
       cases/cleanup/sigint_mid_run.work
       See "The Report" in the manual. */

    /* TODO: Write the average line.
       Average each column over the workers that carry it.
       Use two decimals for the tick averages and six for the millisecond averages.
       cases/worked/stcf.work
       See "The Report" in the manual. */

    /* TODO: Write the result line as finished, interrupted, or faulted.
       cases/fault/crash_mid_burst.work
       See "The Report" in the manual. */

    (void)run;
    (void)out;

    return PS1_OK;
}
