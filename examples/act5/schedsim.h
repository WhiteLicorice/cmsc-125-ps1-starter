/*
 * schedsim.h
 *
 * Shared declarations for the CMSC 125 activity 5 scheduling simulator.
 *
 * The program that links schedsim.c supplies the case list. The student-facing
 * build supplies the demo and worked-example cases in demo_cases.c. Another
 * build can supply a different list.
 */

#ifndef SCHEDSIM_H
#define SCHEDSIM_H

#define MAX_JOBS 8

enum policy { POLICY_FIFO, POLICY_SJF, POLICY_STCF };

struct job {
    char name;
    int arrival;
    int burst;
};

struct case_def {
    const char *id;
    const char *policy_name;
    enum policy policy;
    int cell;
    int count;
    struct job jobs[MAX_JOBS];
};

extern const struct case_def sim_cases[];
extern const int sim_case_count;

#endif
