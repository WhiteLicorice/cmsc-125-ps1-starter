/*
 * sim_engine.c
 *
 * A round-robin schedule simulator for CMSC 125.
 *
 * One job has an arrival time and a list of phases. A phase is either CPU
 * work or I/O work. The engine runs a round-robin scheduler over the jobs.
 *
 * The model is fixed, so every case gives one answer.
 *
 *   - The time unit is one tick. The quantum and the switch cost are in ticks.
 *   - The ready queue is first-in first-out.
 *   - A job that arrives, or that finishes its I/O, enters the tail of the
 *     ready queue. A preempted job enters the tail after them.
 *   - A job that runs its quantum leaves the CPU. A job that finishes its CPU
 *     phase also leaves the CPU. It then blocks for I/O or finishes.
 *   - A context switch costs no ticks in the schedule. A program reports the
 *     cost separately.
 */
#include "sim_engine.h"

#include <stdio.h>
#include <string.h>

#define MAX_JOBS 8
#define MAX_READY 4096
#define MAX_SEG 4096
#define MAX_TIME 100000

#define NO_JOB (-1)

struct sim {
    int running;
    int qleft;
    int pc[MAX_JOBS];
    int pc_left[MAX_JOBS];
    int ready[MAX_READY];
    int head;
    int tail;
    int arrived[MAX_JOBS];
    int blocked[MAX_JOBS];
    int io_ready[MAX_JOBS];
    int first_run[MAX_JOBS];
    int finish[MAX_JOBS];
    int done;
    int dispatches;
    int seg_job[MAX_SEG];
    int seg_start[MAX_SEG];
    int seg_end[MAX_SEG];
    int nseg;
};

static void push_ready(struct sim *s, int job)
{
    s->ready[s->tail] = job;
    s->tail++;
}

static int pop_ready(struct sim *s)
{
    int job = s->ready[s->head];
    s->head++;
    if (s->head == s->tail) {
        s->head = 0;
        s->tail = 0;
    }
    return job;
}

static void record_seg(struct sim *s, int job, int t)
{
    if (s->nseg > 0 && s->seg_job[s->nseg - 1] == job
        && s->seg_end[s->nseg - 1] == t) {
        s->seg_end[s->nseg - 1] = t + 1;
    } else {
        s->seg_job[s->nseg] = job;
        s->seg_start[s->nseg] = t;
        s->seg_end[s->nseg] = t + 1;
        s->nseg++;
    }
}

static void simulate(struct sim *s, const struct scenario *sc)
{
    int total = sc->njobs;
    int t = 0;
    int j;

    memset(s, 0, sizeof *s);
    s->running = NO_JOB;
    for (j = 0; j < total; j++) {
        s->pc_left[j] = sc->jobs[j].phases[0].len;
        s->first_run[j] = NO_JOB;
    }

    while (s->done < total && t < MAX_TIME) {
        for (j = 0; j < total; j++) {
            if (s->blocked[j] && s->io_ready[j] == t) {
                s->blocked[j] = 0;
                s->pc[j]++;
                if (s->pc[j] >= sc->jobs[j].nphases) {
                    s->finish[j] = t;
                    s->done++;
                } else {
                    s->pc_left[j] = sc->jobs[j].phases[s->pc[j]].len;
                    push_ready(s, j);
                }
            }
        }
        for (j = 0; j < total; j++) {
            if (!s->arrived[j] && sc->jobs[j].arrival == t) {
                s->arrived[j] = 1;
                push_ready(s, j);
            }
        }
        if (s->running != NO_JOB && s->qleft == 0) {
            push_ready(s, s->running);
            s->running = NO_JOB;
        }
        if (s->running == NO_JOB && s->head < s->tail) {
            s->running = pop_ready(s);
            s->qleft = sc->quantum;
            s->dispatches++;
            if (s->first_run[s->running] == NO_JOB) {
                s->first_run[s->running] = t;
            }
        }

        if (s->done < total) {
            record_seg(s, s->running, t);
        }

        if (s->running != NO_JOB) {
            int r = s->running;
            s->pc_left[r]--;
            s->qleft--;
            if (s->pc_left[r] == 0) {
                s->pc[r]++;
                if (s->pc[r] >= sc->jobs[r].nphases) {
                    s->finish[r] = t + 1;
                    s->running = NO_JOB;
                    s->done++;
                } else if (sc->jobs[r].phases[s->pc[r]].kind == PH_IO) {
                    s->blocked[r] = 1;
                    s->io_ready[r] = t + 1 + sc->jobs[r].phases[s->pc[r]].len;
                    s->running = NO_JOB;
                } else {
                    s->pc_left[r] = sc->jobs[r].phases[s->pc[r]].len;
                }
            }
        }
        t++;
    }
}

int sim_total_cpu(const struct scenario *sc)
{
    int j, p;
    int sum = 0;
    for (j = 0; j < sc->njobs; j++) {
        for (p = 0; p < sc->jobs[j].nphases; p++) {
            if (sc->jobs[j].phases[p].kind == PH_CPU) {
                sum += sc->jobs[j].phases[p].len;
            }
        }
    }
    return sum;
}

int sim_dispatches(const struct scenario *sc)
{
    struct sim s;
    simulate(&s, sc);
    return s.dispatches;
}

static int burst(const struct job *job)
{
    int p;
    int sum = 0;
    for (p = 0; p < job->nphases; p++) {
        if (job->phases[p].kind == PH_CPU) {
            sum += job->phases[p].len;
        }
    }
    return sum;
}

void sim_print_case(const struct scenario *sc)
{
    struct sim s;
    int j;
    int makespan = 0;
    double tt_sum = 0.0;
    double rt_sum = 0.0;

    simulate(&s, sc);

    printf("case: %s\n", sc->name);
    printf("quantum: %d\n", sc->quantum);
    printf("schedule:\n");
    for (j = 0; j < s.nseg; j++) {
        const char *name = s.seg_job[j] == NO_JOB
            ? "idle" : sc->jobs[s.seg_job[j]].name;
        printf("  %d-%d %s\n", s.seg_start[j], s.seg_end[j], name);
    }
    printf("metrics (ticks):\n");
    printf("  pid  at  bt  ft  tt  rt\n");
    for (j = 0; j < sc->njobs; j++) {
        int b = burst(&sc->jobs[j]);
        int tt = s.finish[j] - sc->jobs[j].arrival;
        int rt = s.first_run[j] - sc->jobs[j].arrival;
        if (s.finish[j] > makespan) {
            makespan = s.finish[j];
        }
        tt_sum += (double)tt;
        rt_sum += (double)rt;
        printf("  %-4s %3d %3d %3d %3d %3d\n", sc->jobs[j].name,
               sc->jobs[j].arrival, b, s.finish[j], tt, rt);
    }
    printf("  average turnaround time: %.2f\n", tt_sum / sc->njobs);
    printf("  average response time: %.2f\n", rt_sum / sc->njobs);
    printf("  cpu ticks: %d, makespan: %d\n", sim_total_cpu(sc), makespan);
    if (makespan > 0) {
        double util = 100.0 * (double)sim_total_cpu(sc) / (double)makespan;
        printf("  cpu utilization: %.1f%%\n", util);
    }
    printf("\n");
}
