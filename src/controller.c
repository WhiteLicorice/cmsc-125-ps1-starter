/*
 * controller.c: yours. The scheduler.
 *
 * The controller forks the workers, decides who runs from the work clock,
 * preempts with SIGSTOP, continues with SIGCONT, writes the traces, and reaps
 * every child on every exit path. See "The Controller" and "The Run Contract"
 * in the manual.
 */
#include "ps1.h"

ps1_status ps1_controller_run(const ps1_workload *wl, const ps1_options *opts,
                              ps1_run *run, ps1_diag *diag)
{
    /* TODO: Record each worker's arrival and burst in *run.
       Set its first dispatch tick and finish tick to -1 before the run.
       See "Its Bookkeeping, Per Worker" in the manual. */

    /* TODO: Install the SIGINT handler without SA_RESTART.
       A blocking call then returns EINTR. The controller reads the handler's flag.
       See "Every Exit Path" in the manual. */

    /* TODO: Open the trace writer for opts->trace_dir.
       See "The Trace" in the manual. */

    /* TODO: Write one event for every dispatch, preemption, stop, and completion.
       cases/worked/rr.work
       See "The Trace" in the manual. */

    /* TODO: Create one progress pipe for each worker.
       cases/normal/fifo_single.work
       See "Its Loop" in the manual. */

    /* TODO: Fork every worker before the first dispatch.
       In the child, close the other workers' progress descriptors.
       cases/normal/max_workers.work
       See "Stopping and Continuing" in the manual. */

    /* TODO: Wait for a worker's birth stop before you dispatch that worker.
       A SIGCONT sent before the worker's own stop is lost. The run hangs.
       See "Stopping and Continuing" in the manual. */

    /* TODO: Release every worker whose arrival tick the clock has reached.
       Record its arrival time.
       Write its trace header.
       See "Its Loop" in the manual. */

    /* TODO: When nothing is running, pick a worker by the policy.
       Continue the worker with SIGCONT.
       cases/normal/fifo_single.work
       See "The Policies" in the manual. */

    /* TODO: Read one byte from the running worker's progress pipe.
       Treat the byte as one finished tick on the work clock.
       See "Its Loop" in the manual. */

    /* TODO: Release an arrival at the new clock before you preempt the runner.
       The arriving worker then stands ahead of the one the preemption picks.
       See "Its Loop" in the manual. */

    /* TODO: Complete a worker when its done count reaches its burst.
       Reap the worker.
       Write its complete event.
       cases/normal/fifo_one_tick.work
       See "When a Decision Is Due" in the manual. */

    /* TODO: Under fifo, never preempt the running worker.
       cases/normal/fifo_two.work
       See "First In, First Out" in the manual. */

    /* TODO: Under rr, preempt at the quantum-th byte.
       cases/normal/rr_two.work
       See "Round Robin" in the manual. */

    /* TODO: Under stcf, preempt only when an arrival has strictly less work left.
       A tie keeps the running worker.
       cases/arrival/stcf_preempts.work
       See "Shortest Time to Completion First" in the manual. */

    /* TODO: At a preemption, write the preempt event at the decision tick.
       Send SIGSTOP.
       Wait for the stop with waitpid.
       cases/arrival/rr_with_arrival.work
       See "Stopping and Continuing" in the manual. */

    /* TODO: Drain the overrun bytes from the pipe without blocking.
       Count each byte as a finished tick.
       See "Stopping and Continuing" in the manual. */

    /* TODO: Release every arrival the overrun reached.
       Do this before you requeue the preempted worker.
       cases/arrival/rr_arrival_joins_tail.work
       See "Round Robin" in the manual. */

    /* TODO: Handle completion when the worker's total done count reaches its burst.
       Continue the worker with SIGCONT if it is still stopped.
       Wait for the worker to exit.
       Write the complete event.
       Write the stopped event only for a worker still stopped with work left.
       Treat a worker that leaves short of its burst as a fault.
       See "Stopping and Continuing" in the manual. */

    /* TODO: Put a still-runnable preempted worker at the back of the queue.
       Do this after the drain and the completion or fault decision.
       Skip this step under stcf.
       See "Round Robin" in the manual. */

    /* TODO: Record a worker's response at its first dispatch.
       Record its turnaround when the worker finishes.
       cases/worked/fifo.work
       See "The Measurement Contract" in the manual. */

    /* TODO: On a worker fault, identify the worker.
       Stop dispatching.
       End the rest.
       Reap those workers.
       Return so the report prints and the exit is 70.
       cases/fault/crash_mid_burst.work
       See "Every Exit Path" in the manual. */

    /* TODO: On SIGINT, stop dispatching.
       End every worker.
       Reap those workers.
       Leave a partial report.
       Return so the exit is 130.
       cases/cleanup/sigint_mid_run.work
       See "Every Exit Path" in the manual. */

    /* TODO: Continue a stopped worker with SIGCONT.
       Send SIGTERM.
       Wait PS1_TERM_GRACE_MS.
       Send SIGKILL.
       Reap whatever remains.
       See "Every Exit Path" in the manual. */

    /* TODO: On a failed fork, end the workers already forked.
       Reap those workers.
       See "Every Exit Path" in the manual. */

    (void)wl;
    (void)opts;
    (void)run;

    snprintf(diag->text, sizeof diag->text, "ps1: controller.c is not implemented");
    return PS1_ERR_FAULT;
}
