/*
 * trace.c: the given trace writer. Do not edit.
 *
 * One file per worker, named <name>.trace, inside the trace directory. Each
 * line holds one event: its name, the tick of the work clock, the ticks that
 * worker has completed, and the CLOCK_MONOTONIC seconds of the call. The
 * controller writes the file's header when it releases the worker.
 *
 *   # ps1 trace: worker A arrival 0 burst 10 time 0.000412
 *   dispatch 0 0 0.000500
 *   preempt 5 5 0.010233
 *   stopped 6 6 0.010489
 *   dispatch 6 6 0.010551
 *   complete 20 10 0.031221
 *
 * The writer is called by the controller and never by a worker, so one process
 * owns every file and no two lines interleave.
 */
#include "ps1.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PS1_TRACE_PATH 512

struct ps1_trace {
    int fd[PS1_MAX_WORKERS];
    const ps1_workload *workload;
};

static void fail(ps1_diag *diag, const char *format, ...)
{
    va_list args;

    if (diag == NULL) {
        return;
    }
    va_start(args, format);
    vsnprintf(diag->text, sizeof diag->text, format, args);
    va_end(args);
}

static const char *event_word(ps1_event event)
{
    switch (event) {
    case PS1_EVENT_DISPATCH:
        return "dispatch";
    case PS1_EVENT_PREEMPT:
        return "preempt";
    case PS1_EVENT_STOPPED:
        return "stopped";
    case PS1_EVENT_COMPLETE:
        return "complete";
    }
    return "unknown";
}

static void close_all(struct ps1_trace *t)
{
    size_t i;

    if (t == NULL) {
        return;
    }
    for (i = 0; i < PS1_MAX_WORKERS; i++) {
        if (t->fd[i] > 0) {
            close(t->fd[i]);
            t->fd[i] = -1;
        }
    }
}

ps1_trace *ps1_trace_open(const char *dir, const ps1_workload *wl, ps1_diag *diag)
{
    struct ps1_trace *t;
    char path[PS1_TRACE_PATH];
    size_t i;

    if (dir == NULL || wl == NULL || wl->count == 0) {
        fail(diag, "ps1: no trace directory or no workers");
        return NULL;
    }

    if (mkdir(dir, 0777) != 0 && errno != EEXIST) {
        fail(diag, "ps1: cannot create trace directory '%s'", dir);
        return NULL;
    }

    t = calloc(1, sizeof *t);
    if (t == NULL) {
        fail(diag, "ps1: cannot allocate the trace writer");
        return NULL;
    }
    for (i = 0; i < PS1_MAX_WORKERS; i++) {
        t->fd[i] = -1;
    }
    t->workload = wl;

    for (i = 0; i < wl->count; i++) {
        int n = snprintf(path, sizeof path, "%s/%s.trace", dir, wl->workers[i].name);
        if (n < 0 || (size_t)n >= sizeof path) {
            fail(diag, "ps1: trace path for worker '%s' is too long", wl->workers[i].name);
            close_all(t);
            free(t);
            return NULL;
        }
        t->fd[i] = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (t->fd[i] < 0) {
            fail(diag, "ps1: cannot write the trace file '%s'", path);
            close_all(t);
            free(t);
            return NULL;
        }
    }
    return t;
}

ps1_status ps1_trace_arrival(ps1_trace *t, size_t index, double seconds)
{
    const ps1_worker *worker;

    if (t == NULL || t->workload == NULL || index >= t->workload->count ||
        t->fd[index] < 0) {
        return PS1_ERR_FAULT;
    }
    worker = &t->workload->workers[index];
    dprintf(t->fd[index], "# ps1 trace: worker %s arrival %ld burst %ld time %.6f\n",
            worker->name, worker->arrival, worker->burst, seconds);
    return PS1_OK;
}

ps1_status ps1_trace_event(ps1_trace *t, size_t index, ps1_event event,
                           long tick, long done, double seconds)
{
    if (t == NULL || index >= PS1_MAX_WORKERS || t->fd[index] < 0) {
        return PS1_ERR_FAULT;
    }
    dprintf(t->fd[index], "%s %ld %ld %.6f\n", event_word(event), tick, done, seconds);
    return PS1_OK;
}

void ps1_trace_close(ps1_trace *t)
{
    close_all(t);
    free(t);
}
