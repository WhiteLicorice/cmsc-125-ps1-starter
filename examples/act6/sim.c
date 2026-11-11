/*
 * sim.c
 *
 * The student cases for the CMSC 125 Activity 6 simulator.
 *
 * The program runs a round-robin scheduler over several workloads and prints
 * the schedule and the metrics for each one. Every case here is a worked
 * example from the activity. See README.md.
 *
 * Run one case:
 *
 *   ./sim <case>
 *
 * Run every case:
 *
 *   ./sim
 */
#include "sim_engine.h"

#include <stdio.h>
#include <string.h>

/* One CPU phase. */
static const struct phase ph_5[] = { { PH_CPU, 5 } };
static const struct phase ph_10[] = { { PH_CPU, 10 } };
static const struct phase ph_20[] = { { PH_CPU, 20 } };
static const struct phase ph_30[] = { { PH_CPU, 30 } };
static const struct phase ph_40[] = { { PH_CPU, 40 } };
static const struct phase ph_60[] = { { PH_CPU, 60 } };
static const struct phase ph_120[] = { { PH_CPU, 120 } };

/* Five rounds of ten ticks of CPU work, with ten ticks of I/O between. */
static const struct phase ph_io10x5[] = {
    { PH_CPU, 10 }, { PH_IO, 10 }, { PH_CPU, 10 }, { PH_IO, 10 },
    { PH_CPU, 10 }, { PH_IO, 10 }, { PH_CPU, 10 }, { PH_IO, 10 },
    { PH_CPU, 10 }, { PH_IO, 10 },
};

/* Three jobs of equal length that all arrive at once. */
static const struct job equal_jobs[] = {
    { "A", 0, 1, ph_5 },
    { "B", 0, 1, ph_5 },
    { "C", 0, 1, ph_5 },
};

/* Three jobs of unequal length that all arrive at once. */
static const struct job mixed_jobs[] = {
    { "124", 0, 1, ph_120 },
    { "131", 0, 1, ph_60 },
    { "141", 0, 1, ph_30 },
};

/* Four jobs that arrive one after another. */
static const struct job arrival_jobs[] = {
    { "A", 0, 1, ph_40 },
    { "B", 10, 1, ph_30 },
    { "C", 20, 1, ph_20 },
    { "D", 30, 1, ph_10 },
};

/* One job that blocks for I/O after every ten ticks of CPU work. */
static const struct job io_alone_jobs[] = {
    { "A", 0, 10, ph_io10x5 },
};

/* Six equal jobs for the switch-cost case. */
static const struct job switch_jobs[] = {
    { "J1", 0, 1, ph_20 }, { "J2", 0, 1, ph_20 }, { "J3", 0, 1, ph_20 },
    { "J4", 0, 1, ph_20 }, { "J5", 0, 1, ph_20 }, { "J6", 0, 1, ph_20 },
};

static const struct scenario scenarios[] = {
    { "equal-q1",    1, 0, 3, equal_jobs },
    { "equal-q5",    5, 0, 3, equal_jobs },
    { "mixed-q5",    5, 0, 3, mixed_jobs },
    { "arrivals-q20", 20, 0, 4, arrival_jobs },
    { "io-alone",   10, 0, 1, io_alone_jobs },
};

#define NCASES (int)(sizeof scenarios / sizeof scenarios[0])

static void print_switch_cost(void)
{
    static const int quanta[] = { 1, 5, 10, 20 };
    const int c = 1;
    struct scenario sc = { "switch-cost", 0, c, 6, switch_jobs };
    int cpu = sim_total_cpu(&sc);
    int i;

    printf("case: switch-cost\n");
    printf("workload: six jobs, 20 ticks of CPU work each, all arrive at tick 0\n");
    printf("switch cost: %d tick\n", c);
    printf("  quantum  dispatches  switches  cpu  switch  total  overhead\n");
    for (i = 0; i < (int)(sizeof quanta / sizeof quanta[0]); i++) {
        int switches;
        int switch_ticks;
        int total;

        sc.quantum = quanta[i];
        switches = sim_dispatches(&sc) - 1;
        switch_ticks = switches * c;
        total = cpu + switch_ticks;
        printf("  %-7d  %-10d  %-8d  %-3d  %-6d  %-5d  %.1f%%\n",
               quanta[i], sim_dispatches(&sc), switches, cpu, switch_ticks,
               total, 100.0 * (double)switch_ticks / (double)total);
    }
    printf("\n");
}

static const struct scenario *find_case(const char *name)
{
    int i;
    for (i = 0; i < NCASES; i++) {
        if (strcmp(scenarios[i].name, name) == 0) {
            return &scenarios[i];
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    int i;
    if (argc > 2) {
        fprintf(stderr, "usage: %s [case]\n", argv[0]);
        return 2;
    }
    if (argc == 2) {
        const struct scenario *sc;
        if (strcmp(argv[1], "switch-cost") == 0) {
            print_switch_cost();
            return 0;
        }
        sc = find_case(argv[1]);
        if (sc == NULL) {
            fprintf(stderr, "unknown case: %s\n", argv[1]);
            return 2;
        }
        sim_print_case(sc);
        return 0;
    }
    for (i = 0; i < NCASES; i++) {
        sim_print_case(&scenarios[i]);
    }
    print_switch_cost();
    return 0;
}
