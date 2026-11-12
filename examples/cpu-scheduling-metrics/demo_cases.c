/*
 * demo_cases.c
 *
 * The student-facing case list for the CPU-scheduling simulator. Every case
 * here appears in the worked examples.
 *
 * See README.md in this folder for the model and the case table.
 */

#include "schedsim.h"

const struct case_def sim_cases[] = {
    { "fifo-basic",    "FIFO", POLICY_FIFO, 1,  3,
      { {'A', 0, 10}, {'B', 0, 10}, {'C', 0, 10} } },
    { "fifo-convoy",   "FIFO", POLICY_FIFO, 10, 3,
      { {'A', 0, 120}, {'B', 0, 60}, {'C', 0, 30} } },
    { "sjf-convoy",    "SJF",  POLICY_SJF,  10, 3,
      { {'A', 0, 120}, {'B', 0, 60}, {'C', 0, 30} } },
    { "fifo-arrivals", "FIFO", POLICY_FIFO, 10, 3,
      { {'A', 0, 120}, {'B', 10, 60}, {'C', 10, 30} } },
    { "sjf-arrivals",  "SJF",  POLICY_SJF,  10, 3,
      { {'A', 0, 120}, {'B', 10, 60}, {'C', 10, 30} } },
    { "stcf-arrivals", "STCF", POLICY_STCF, 10, 3,
      { {'A', 0, 120}, {'B', 10, 60}, {'C', 10, 30} } },
    { "stcf-same",     "STCF", POLICY_STCF, 10, 3,
      { {'A', 0, 120}, {'B', 0, 60}, {'C', 0, 30} } },
};

const int sim_case_count =
    (int)(sizeof(sim_cases) / sizeof(sim_cases[0]));
