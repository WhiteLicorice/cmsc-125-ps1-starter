/*
 * sim_engine.h
 *
 * The scheduler engine for the CMSC 125 Activity 6 simulator.
 *
 * A program describes a workload as a list of scenarios, then calls
 * sim_print_case on each one. See sim_engine.c for the model.
 */
#ifndef SIM_ENGINE_H
#define SIM_ENGINE_H

enum { PH_CPU = 0, PH_IO = 1 };

struct phase {
    int kind;
    int len;
};

struct job {
    const char *name;
    int arrival;
    int nphases;
    const struct phase *phases;
};

struct scenario {
    const char *name;
    int quantum;
    int switch_cost;
    int njobs;
    const struct job *jobs;
};

/* Print the schedule and the metrics for one scenario. */
void sim_print_case(const struct scenario *sc);

/* Return the number of dispatches in one scenario. */
int sim_dispatches(const struct scenario *sc);

/* Return the total CPU ticks in one scenario. */
int sim_total_cpu(const struct scenario *sc);

#endif
