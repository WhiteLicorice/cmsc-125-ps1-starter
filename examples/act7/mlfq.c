/*
 * mlfq.c
 *
 * A deterministic multi-level feedback queue scheduler.
 *
 * The program prints one workload's schedule and its metrics. The model is
 * the one the activity defines:
 *
 *   - The time unit is 1 ms.
 *   - The scheduler has three queues, with Q1 at the top and Q3 at the bottom.
 *   - Each queue uses round-robin scheduling with slices of 2, 4, or 8 ms.
 *   - A job moves down when it uses its level's allotment across all runs at
 *     that level. A job in the bottom queue does not move down.
 *   - A new job enters at the tail of Q1.
 *   - A job that stops running returns to the tail of its queue. It starts
 *     a fresh slice when it runs again.
 *   - A job is preempted only when another job arrives in a strictly
 *     higher queue, or at a boost.
 *   - A boost moves every unfinished job to Q1 at the end of each period.
 *   - Switching costs no time. The program prints each idle millisecond.
 *
 * The accounting option selects the demotion rule. "allotment" is the rule
 * the activity settles on. "slice" is the earlier rule, which demotes a job
 * only when it uses a whole slice in one run.
 *
 * Build:
 *   cc -std=c11 -Wall -Wextra -Werror mlfq.c -o mlfq
 *
 * Usage:
 *   ./mlfq --case learning
 *
 * The five cases are learning, gaming-old, gaming-new, boost-off, and
 * boost-on. Each one prints the schedule and the metrics for its workload.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_JOBS   32
#define MAX_LEVELS 4
#define MAX_ARGS   64

enum { NOT_ARRIVED, WAITING, FINISHED };

enum { ACCT_ALLOTMENT, ACCT_SLICE };

struct job {
    char id[8];
    int  arrival;
    int  cpu;
    int  yield_after;
    int  remaining;
    int  state;
    int  level;
    int  level_time;
    int  run_slice;
    int  run_since_yield;
    int  first_run;
    int  finish;
};

struct model {
    char name[64];
    int  levels;
    int  slice[MAX_LEVELS];
    int  allot[MAX_LEVELS];
    int  boost;
    int  accounting;
    int  njobs;
    struct job jobs[MAX_JOBS];
};

struct sim {
    struct model *m;
    int  queue[MAX_LEVELS][MAX_JOBS];
    int  count[MAX_LEVELS];
    int  current;
    int  ticks;
    int *tick_job;
    int *tick_level;
    int  cap;
};

static void push(struct sim *s, int level, int job)
{
    s->queue[level][s->count[level]++] = job;
}

static int pop(struct sim *s, int level)
{
    int job = s->queue[level][0];

    for (int i = 1; i < s->count[level]; i++)
        s->queue[level][i - 1] = s->queue[level][i];
    s->count[level]--;

    return job;
}

static int pick(struct sim *s)
{
    for (int level = 0; level < s->m->levels; level++)
        if (s->count[level] > 0)
            return pop(s, level);

    return -1;
}

/* Put the running job at the tail of its queue. It keeps its level time. */
static void stop(struct sim *s, int index)
{
    struct job *j = &s->m->jobs[index];

    j->run_slice = 0;
    j->run_since_yield = 0;
    j->state = WAITING;
    push(s, j->level, index);
    s->current = -1;
}

/* Move the running job one queue down, and reset its level time. */
static void demote(struct sim *s, int index)
{
    struct job *j = &s->m->jobs[index];

    if (j->level < s->m->levels - 1)
        j->level++;
    j->level_time = 0;
    stop(s, index);
}

/* Move every unfinished job to Q1. The running job joins the head of its
 * own queue first, and the queues merge in order from Q1 down. */
static void boost(struct sim *s)
{
    int merged[MAX_JOBS];
    int n = 0;

    if (s->current >= 0) {
        struct job *j = &s->m->jobs[s->current];

        for (int i = s->count[j->level]; i > 0; i--)
            s->queue[j->level][i] = s->queue[j->level][i - 1];
        s->queue[j->level][0] = s->current;
        s->count[j->level]++;
        j->state = WAITING;
        s->current = -1;
    }

    for (int level = 0; level < s->m->levels; level++)
        while (s->count[level] > 0)
            merged[n++] = pop(s, level);

    for (int i = 0; i < n; i++) {
        struct job *j = &s->m->jobs[merged[i]];

        j->level = 0;
        j->level_time = 0;
        j->run_slice = 0;
        j->run_since_yield = 0;
        push(s, 0, merged[i]);
    }
}

static void simulate(struct sim *s)
{
    struct model *m = s->m;
    int done = 0;
    int t = 0;

    s->current = -1;

    while (done < m->njobs) {
        if (t >= s->cap) {
            fprintf(stderr, "mlfq: the trace is longer than the buffer\n");
            exit(1);
        }

        for (int i = 0; i < m->njobs; i++) {
            struct job *j = &m->jobs[i];

            if (j->state == NOT_ARRIVED && j->arrival == t) {
                j->state = WAITING;
                j->level = 0;
                j->level_time = 0;
                j->run_slice = 0;
                j->run_since_yield = 0;
                push(s, 0, i);
            }
        }

        if (m->boost > 0 && t > 0 && t % m->boost == 0)
            boost(s);

        if (s->current >= 0) {
            int level = m->jobs[s->current].level;

            for (int lower = 0; lower < level; lower++) {
                if (s->count[lower] > 0) {
                    stop(s, s->current);
                    break;
                }
            }
        }

        if (s->current < 0)
            s->current = pick(s);

        if (s->current < 0) {
            s->tick_job[t] = -1;
            s->tick_level[t] = -1;
            t++;
            continue;
        }

        int index = s->current;
        struct job *j = &m->jobs[index];

        if (j->first_run < 0)
            j->first_run = t;
        j->remaining--;
        j->level_time++;
        j->run_slice++;
        j->run_since_yield++;
        s->tick_job[t] = index;
        s->tick_level[t] = j->level;

        if (j->remaining == 0) {
            j->state = FINISHED;
            j->finish = t + 1;
            s->current = -1;
            done++;
        } else if (j->run_slice == m->slice[j->level]) {
            if (m->accounting == ACCT_SLICE)
                demote(s, index);
            else if (m->allot[j->level] > 0 && j->level_time >= m->allot[j->level])
                demote(s, index);
            else
                stop(s, index);
        } else if (m->accounting == ACCT_ALLOTMENT
                   && m->allot[j->level] > 0 && j->level_time >= m->allot[j->level]) {
            demote(s, index);
        } else if (j->yield_after > 0 && j->run_since_yield == j->yield_after) {
            stop(s, index);
        }

        t++;
    }

    s->ticks = t;
}

static void print_list(char *out, size_t size, const int *values, int count)
{
    size_t used = 0;

    out[0] = '\0';
    for (int i = 0; i < count; i++) {
        int written = snprintf(out + used, size - used, "%s%d",
                               i == 0 ? "" : "/", values[i]);

        if (written < 0 || (size_t)written >= size - used)
            return;
        used += (size_t)written;
    }
}

static void report(struct sim *s)
{
    struct model *m = s->m;
    char slices[64];
    char allots[64];
    char average[2][16];

    print_list(slices, sizeof slices, m->slice, m->levels);
    print_list(allots, sizeof allots, m->allot, m->levels - 1);

    printf("case: %s\n", m->name);
    printf("model: queues %d, slice %s ms, allotment %s ms, boost ",
           m->levels, slices, allots);
    if (m->boost > 0)
        printf("every %d ms", m->boost);
    else
        printf("off");
    printf(", accounting %s, no switch cost\n",
           m->accounting == ACCT_SLICE ? "slice" : "allotment");

    printf("jobs:");
    for (int i = 0; i < m->njobs; i++) {
        struct job *j = &m->jobs[i];

        printf(" %s(arr %d cpu %d", j->id, j->arrival, j->cpu);
        if (j->yield_after > 0)
            printf(" yield %d", j->yield_after);
        printf(")");
    }
    printf("\n\n");

    printf("schedule\n");
    for (int i = 0; i < s->ticks;) {
        int job = s->tick_job[i];
        int level = s->tick_level[i];
        int last = i;
        char span[32];

        while (last + 1 < s->ticks
               && s->tick_job[last + 1] == job
               && s->tick_level[last + 1] == level)
            last++;

        snprintf(span, sizeof span, "%d-%d", i, last + 1);
        if (job < 0)
            printf("  %-8s idle\n", span);
        else
            printf("  %-8s %-4s Q%d\n", span, m->jobs[job].id, level + 1);

        i = last + 1;
    }

    printf("\nmetrics\n");
    printf("%-6s %4s %7s %11s %9s\n", "job", "arr", "finish",
           "turnaround", "response");

    double sum_turnaround = 0.0;
    double sum_response = 0.0;

    for (int i = 0; i < m->njobs; i++) {
        struct job *j = &m->jobs[i];
        int turnaround = j->finish - j->arrival;
        int response = j->first_run - j->arrival;

        sum_turnaround += turnaround;
        sum_response += response;
        printf("%-6s %4d %7d %11d %9d\n", j->id, j->arrival, j->finish,
               turnaround, response);
    }

    snprintf(average[0], sizeof average[0], "%.2f", sum_turnaround / m->njobs);
    snprintf(average[1], sizeof average[1], "%.2f", sum_response / m->njobs);
    printf("%-6s %4s %7s %11s %9s\n", "average", "", "", average[0], average[1]);
}

static int parse_numbers(const char *text, int *out, int maximum)
{
    int n = 0;
    const char *p = text;

    while (*p != '\0') {
        char *end = NULL;
        long value = strtol(p, &end, 10);

        if (end == p || value < 0)
            return -1;
        if (n >= maximum)
            return -1;
        out[n++] = (int)value;
        p = end;
        if (*p == ',')
            p++;
        else if (*p != '\0')
            return -1;
    }

    return n;
}

static int parse_job(struct job *j, char *text)
{
    char *fields[4];
    int n = 0;

    fields[n++] = text;
    for (char *p = text; *p != '\0'; p++) {
        if (*p == ',') {
            *p = '\0';
            if (n >= 4)
                return -1;
            fields[n++] = p + 1;
        }
    }
    if (n < 3 || n > 4)
        return -1;

    snprintf(j->id, sizeof j->id, "%s", fields[0]);
    j->arrival = atoi(fields[1]);
    j->cpu = atoi(fields[2]);
    j->yield_after = n == 4 ? atoi(fields[3]) : 0;
    if (j->arrival < 0 || j->cpu <= 0 || j->yield_after < 0)
        return -1;

    return 0;
}

static int parse(struct model *m, int argc, char **argv)
{
    m->levels = 3;
    m->slice[0] = 2;
    m->slice[1] = 4;
    m->slice[2] = 8;
    m->slice[3] = 0;
    m->allot[0] = 4;
    m->allot[1] = 8;
    m->allot[2] = 0;
    m->allot[3] = 0;
    m->boost = 0;
    m->accounting = ACCT_ALLOTMENT;
    m->njobs = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--slice") == 0 && i + 1 < argc) {
            int n = parse_numbers(argv[++i], m->slice, MAX_LEVELS);

            if (n < 1)
                return -1;
            m->levels = n;
        } else if (strcmp(argv[i], "--allot") == 0 && i + 1 < argc) {
            int values[MAX_LEVELS];

            int n = parse_numbers(argv[++i], values, MAX_LEVELS);

            if (n < 0)
                return -1;
            for (int k = 0; k < MAX_LEVELS; k++)
                m->allot[k] = k < n ? values[k] : 0;
        } else if (strcmp(argv[i], "--boost") == 0 && i + 1 < argc) {
            m->boost = atoi(argv[++i]);
            if (m->boost < 0)
                return -1;
        } else if (strcmp(argv[i], "--accounting") == 0 && i + 1 < argc) {
            const char *which = argv[++i];

            if (strcmp(which, "allotment") == 0)
                m->accounting = ACCT_ALLOTMENT;
            else if (strcmp(which, "slice") == 0)
                m->accounting = ACCT_SLICE;
            else
                return -1;
        } else if (strcmp(argv[i], "--job") == 0 && i + 1 < argc) {
            if (m->njobs >= MAX_JOBS)
                return -1;
            if (parse_job(&m->jobs[m->njobs], argv[++i]) != 0)
                return -1;
            m->njobs++;
        } else if (strcmp(argv[i], "--name") == 0 && i + 1 < argc) {
            snprintf(m->name, sizeof m->name, "%s", argv[++i]);
        } else {
            return -1;
        }
    }

    if (m->njobs == 0)
        return -1;

    return 0;
}

static const char *case_body(const char *name)
{
    static const struct {
        const char *name;
        const char *body;
    } cases[] = {
        { "learning",
          "--slice 2,4,8 --allot 4,8 --boost 0 --accounting allotment"
          " --job A,0,20 --job B,5,3 --job C,6,3" },
        { "gaming-old",
          "--slice 2,4,8 --allot 4,8 --boost 0 --accounting slice"
          " --job G,0,20,1 --job H,0,4" },
        { "gaming-new",
          "--slice 2,4,8 --allot 4,8 --boost 0 --accounting allotment"
          " --job G,0,20,1 --job H,0,4" },
        { "boost-off",
          "--slice 2,4,8 --allot 4,8 --boost 0 --accounting allotment"
          " --job L,0,8 --job s1,4,4 --job s2,8,4 --job s3,12,4" },
        { "boost-on",
          "--slice 2,4,8 --allot 4,8 --boost 8 --accounting allotment"
          " --job L,0,8 --job s1,4,4 --job s2,8,4 --job s3,12,4" },
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; i++)
        if (strcmp(cases[i].name, name) == 0)
            return cases[i].body;

    return NULL;
}

static int split_words(char *line, char **out, int maximum)
{
    int n = 0;
    char *p = line;

    while (*p != '\0') {
        while (*p == ' ')
            p++;
        if (*p == '\0')
            break;
        if (n >= maximum)
            return -1;
        out[n++] = p;
        while (*p != '\0' && *p != ' ')
            p++;
        if (*p != '\0')
            *p++ = '\0';
    }

    return n;
}

static void usage(void)
{
    fprintf(stderr,
            "usage: mlfq --case NAME\n"
            "\n"
            "cases: learning gaming-old gaming-new boost-off boost-on\n");
}

/* Run one model. Print its schedule and its metrics. */
static int run_model(struct model *m)
{
    struct sim s;

    memset(&s, 0, sizeof s);
    s.m = m;
    s.cap = 16;
    for (int i = 0; i < m->njobs; i++) {
        s.cap += m->jobs[i].cpu + m->jobs[i].arrival;
        m->jobs[i].state = NOT_ARRIVED;
        m->jobs[i].level = -1;
        m->jobs[i].remaining = m->jobs[i].cpu;
        m->jobs[i].first_run = -1;
        m->jobs[i].finish = -1;
    }
    s.tick_job = calloc((size_t)s.cap, sizeof *s.tick_job);
    s.tick_level = calloc((size_t)s.cap, sizeof *s.tick_level);
    if (s.tick_job == NULL || s.tick_level == NULL) {
        free(s.tick_job);
        free(s.tick_level);
        fprintf(stderr, "mlfq: out of memory\n");
        return -1;
    }

    simulate(&s);
    report(&s);

    free(s.tick_job);
    free(s.tick_level);

    return 0;
}

int main(int argc, char **argv)
{
    struct model model;
    char body[512];
    char *expanded[MAX_ARGS];
    int expanded_count = 0;
    int added;
    const char *source;

    if (argc != 3 || strcmp(argv[1], "--case") != 0) {
        usage();
        return 1;
    }

    source = case_body(argv[2]);
    if (source == NULL) {
        fprintf(stderr, "mlfq: no case named %s\n", argv[2]);
        return 1;
    }

    memset(&model, 0, sizeof model);
    snprintf(model.name, sizeof model.name, "%s", argv[2]);
    snprintf(body, sizeof body, "%s", source);

    expanded[expanded_count++] = argv[0];
    added = split_words(body, expanded + expanded_count,
                        MAX_ARGS - expanded_count);
    if (added < 0) {
        fprintf(stderr, "mlfq: the case has too many arguments\n");
        return 1;
    }
    expanded_count += added;

    if (parse(&model, expanded_count, expanded) != 0) {
        fprintf(stderr, "mlfq: the case is not valid\n");
        return 1;
    }

    return run_model(&model) == 0 ? 0 : 1;
}
