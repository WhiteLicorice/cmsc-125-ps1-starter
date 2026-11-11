/*
 * schedsim.c
 *
 * A deterministic CPU-scheduling simulator for CMSC 125 activity 5.
 *
 * This file holds the model and the printer. The case list is linked in from
 * the build. One case is one policy on one workload. For each case the program
 * prints the schedule, a Gantt row, and the per-job metrics. The model is
 * fixed. examples/README.md states it.
 *
 * The student-facing build links demo_cases.c:
 *   cc -std=c11 -Wall -Wextra -Werror -o schedsim schedsim.c demo_cases.c
 *
 * Run one case:
 *   ./schedsim fifo-arrivals
 *
 * Run every case:
 *   ./schedsim
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "schedsim.h"

#define MAX_SEGS 32

struct seg {
    int start;
    int end;
    int job;
};

static int pick(const struct case_def *c, const int *remaining, int time,
                int running)
{
    int best = -1;
    int i;

    for (i = 0; i < c->count; i++) {
        if (c->jobs[i].arrival > time || remaining[i] == 0)
            continue;
        if (best < 0) {
            best = i;
            continue;
        }
        if (c->policy == POLICY_FIFO &&
            c->jobs[i].arrival < c->jobs[best].arrival)
            best = i;
        if (c->policy == POLICY_SJF &&
            c->jobs[i].burst < c->jobs[best].burst)
            best = i;
        if (c->policy == POLICY_STCF && remaining[i] < remaining[best])
            best = i;
    }

    if (c->policy == POLICY_STCF && best >= 0 && running >= 0 &&
        remaining[running] == remaining[best])
        best = running;

    return best;
}

static int simulate(const struct case_def *c, struct seg *segs, int *nsegs,
                    int *completion, int *first_run)
{
    int remaining[MAX_JOBS];
    int time = 0;
    int running = -1;
    int done = 0;
    int seg_count = 0;
    int i;

    for (i = 0; i < c->count; i++) {
        remaining[i] = c->jobs[i].burst;
        completion[i] = 0;
        first_run[i] = -1;
    }

    while (done < c->count) {
        int j = pick(c, remaining, time, running);

        if (j < 0) {
            time++;
            continue;
        }
        if (c->policy != POLICY_STCF && running >= 0)
            j = running;

        if (first_run[j] < 0)
            first_run[j] = time;
        remaining[j]--;

        if (seg_count > 0 && segs[seg_count - 1].job == j &&
            segs[seg_count - 1].end == time)
            segs[seg_count - 1].end = time + 1;
        else {
            segs[seg_count].start = time;
            segs[seg_count].end = time + 1;
            segs[seg_count].job = j;
            seg_count++;
        }

        time++;
        if (remaining[j] == 0) {
            completion[j] = time;
            done++;
            running = -1;
        } else {
            running = j;
        }
    }

    *nsegs = seg_count;
    return time;
}

static void print_case(const struct case_def *c)
{
    struct seg segs[MAX_SEGS];
    int completion[MAX_JOBS];
    int first_run[MAX_JOBS];
    int nsegs = 0;
    int makespan;
    int cols;
    int i;
    int k;

    makespan = simulate(c, segs, &nsegs, completion, first_run);
    cols = makespan / c->cell;

    printf("case %s\n", c->id);
    printf("policy %s\n", c->policy_name);
    printf("workload");
    for (i = 0; i < c->count; i++)
        printf(" %c:%d/%d", c->jobs[i].name, c->jobs[i].arrival,
               c->jobs[i].burst);
    printf("\n\n");

    printf("schedule\n");
    for (k = 0; k < nsegs; k++)
        printf("  %d-%d %c\n", segs[k].start, segs[k].end,
               c->jobs[segs[k].job].name);
    printf("\n");

    printf("gantt (1 column = %d time unit%s)\n  ", c->cell,
           c->cell == 1 ? "" : "s");
    for (k = 0; k < cols; k++) {
        int t = k * c->cell;
        int run = -1;
        int s;
        for (s = 0; s < nsegs; s++)
            if (segs[s].start <= t && t < segs[s].end)
                run = segs[s].job;
        putchar(run < 0 ? '.' : c->jobs[run].name);
    }
    printf("\n\n");

    printf("metrics\n");
    printf("  %-3s %-7s %-5s %-10s %-10s %s\n", "job", "arrival", "burst",
           "completion", "turnaround", "response");
    {
        int sum_tat = 0;
        int sum_resp = 0;
        for (i = 0; i < c->count; i++) {
            int tat = completion[i] - c->jobs[i].arrival;
            int resp = first_run[i] - c->jobs[i].arrival;
            printf("  %-3c %-7d %-5d %-10d %-10d %d\n", c->jobs[i].name,
                   c->jobs[i].arrival, c->jobs[i].burst, completion[i], tat,
                   resp);
            sum_tat += tat;
            sum_resp += resp;
        }
        printf("  %-3s %-7s %-5s %-10s %-10.2f %.2f\n", "avg", "", "", "",
               (double)sum_tat / c->count, (double)sum_resp / c->count);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    size_t ncases = (size_t)sim_case_count;
    size_t i;

    if (argc == 1) {
        for (i = 0; i < ncases; i++)
            print_case(&sim_cases[i]);
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "usage: %s [case]\n", argv[0]);
        return 2;
    }
    for (i = 0; i < ncases; i++) {
        if (strcmp(argv[1], sim_cases[i].id) == 0) {
            print_case(&sim_cases[i]);
            return 0;
        }
    }
    fprintf(stderr, "unknown case: %s\n", argv[1]);
    return 2;
}
