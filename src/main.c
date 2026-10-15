/*
 * main.c: the given program entry point. Do not edit.
 *
 * It reads the command line, loads the workload, runs the controller, prints
 * the report, and maps the result onto the exit codes the manual states.
 *
 *   0    every worker finished and every reference was released
 *   65   the command line or the workload file was refused
 *   70   a worker failed, or a system call did
 *   130  SIGINT ended the run
 */
#include "ps1.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void usage(void)
{
    fprintf(stderr,
            "usage: cmsc-125-ps1 [--policy fifo|rr|stcf] [--quantum N] "
            "[--trace-dir DIR] <workload>\n");
}

static int parse_positive_long(const char *text, long *out)
{
    char *end = NULL;
    long value;

    if (text == NULL || *text == '\0') {
        return -1;
    }
    errno = 0;
    value = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return -1;
    }
    if (value <= 0 || value > PS1_MAX_TICKS) {
        return -1;
    }
    *out = value;
    return 0;
}

static int set_policy(const char *word, ps1_options *opts)
{
    if (strcmp(word, "fifo") == 0) {
        opts->policy = PS1_POLICY_FIFO;
    } else if (strcmp(word, "rr") == 0) {
        opts->policy = PS1_POLICY_RR;
    } else if (strcmp(word, "stcf") == 0) {
        opts->policy = PS1_POLICY_STCF;
    } else {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    ps1_options opts;
    ps1_workload workload;
    ps1_run run;
    ps1_diag diag;
    ps1_status status;
    int policy_given = 0;
    int quantum_given = 0;
    int i;

    memset(&opts, 0, sizeof opts);
    memset(&diag, 0, sizeof diag);

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--policy") == 0 || strcmp(arg, "--quantum") == 0 ||
            strcmp(arg, "--trace-dir") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ps1: %s needs a value\n", arg);
                usage();
                return PS1_EXIT_REJECTED;
            }
        }

        if (strcmp(arg, "--policy") == 0) {
            if (set_policy(argv[++i], &opts) != 0) {
                fprintf(stderr, "ps1: --policy needs fifo, rr, or stcf\n");
                return PS1_EXIT_REJECTED;
            }
            policy_given = 1;
        } else if (strcmp(arg, "--quantum") == 0) {
            if (parse_positive_long(argv[++i], &opts.quantum) != 0) {
                fprintf(stderr, "ps1: --quantum needs a number from 1 to %d\n", PS1_MAX_TICKS);
                return PS1_EXIT_REJECTED;
            }
            quantum_given = 1;
        } else if (strcmp(arg, "--trace-dir") == 0) {
            opts.trace_dir = argv[++i];
        } else if (arg[0] == '-' && arg[1] != '\0') {
            fprintf(stderr, "ps1: unknown option '%s'\n", arg);
            usage();
            return PS1_EXIT_REJECTED;
        } else if (opts.path == NULL) {
            opts.path = arg;
        } else {
            fprintf(stderr, "ps1: unexpected argument '%s'\n", arg);
            usage();
            return PS1_EXIT_REJECTED;
        }
    }

    if (!policy_given) {
        fprintf(stderr, "ps1: --policy is required\n");
        return PS1_EXIT_REJECTED;
    }
    if (opts.policy == PS1_POLICY_RR && !quantum_given) {
        fprintf(stderr, "ps1: --quantum is required for policy rr\n");
        return PS1_EXIT_REJECTED;
    }
    if (opts.policy != PS1_POLICY_RR && quantum_given) {
        fprintf(stderr, "ps1: --quantum applies only to policy rr\n");
        return PS1_EXIT_REJECTED;
    }
    if (opts.path == NULL) {
        fprintf(stderr, "ps1: no workload file\n");
        usage();
        return PS1_EXIT_REJECTED;
    }
    if (opts.trace_dir == NULL) {
        opts.trace_dir = "traces";
    }

    status = ps1_workload_load(opts.path, &workload, &diag);
    if (status != PS1_OK) {
        fprintf(stderr, "%s\n", diag.text);
        return PS1_EXIT_REJECTED;
    }

    memset(&run, 0, sizeof run);
    run.workload = &workload;
    run.options = &opts;
    run.fault_worker = (size_t)-1;

    status = ps1_controller_run(&workload, &opts, &run, &diag);

    if (ps1_report_print(&run, stdout) != PS1_OK) {
        fprintf(stderr, "ps1: cannot write the report\n");
        return PS1_EXIT_FAULT;
    }
    fflush(stdout);

    if (status == PS1_OK) {
        return PS1_EXIT_OK;
    }
    if (status == PS1_ERR_INTERRUPTED) {
        fprintf(stderr, "%s\n", diag.text);
        return PS1_EXIT_INTERRUPTED;
    }
    fprintf(stderr, "%s\n", diag.text);
    return PS1_EXIT_FAULT;
}
