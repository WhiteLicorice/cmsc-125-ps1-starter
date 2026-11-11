<!--no-pdf-->
# Activity 7 Examples

This folder holds one program. `mlfq.c` is a multilevel feedback queue
scheduler. It implements the model the activity defines. It prints the
schedule and the metrics for one workload. The program is deterministic. The
same command prints the same output on every machine.

The program runs on Linux. On Windows, run it inside Ubuntu under Windows
Subsystem for Linux (WSL2). On macOS, run it inside the Ubuntu 24.04 Server
virtual machine from the `b1` manual, Part 4, run with UTM. On Linux, run it in
your own terminal. Run every command inside the Ubuntu guest, on Windows or on
a Mac.

## Requirements

- A C compiler. The outputs below come from `cc` in GCC 13.3.0.
- `make`.

## Build and Run

Build the simulator.

    make

Build the simulator. Run every case. Compare the output.

    make check

Delete the simulator.

    make clean

Build the simulator with the compiler directly.

    cc -std=c11 -Wall -Wextra -Werror -o mlfq mlfq.c

The compile must stay clean. `-Werror` turns every warning into an error.

## How the Check Decides

`make check` runs each case in turn. It compares the output with
`expected/<case>.txt`. The simulator is deterministic, so every comparison is
exact. It prints one line for each case, then a count line. It exits 0 when
every case passes. It exits nonzero when a case fails.

## The Model

The simulator implements the model below. The activity states the same model.

- The time unit is 1 ms. Every time below is a whole number of milliseconds.
- Three queues hold the ready jobs. Q1 is the top queue and Q3 is the bottom
  queue.
- A queue runs its jobs in round robin. The slice is 2 ms at Q1, 4 ms at Q2,
  and 8 ms at Q3.
- The scheduler demotes a job when its level time reaches the level's
  **allotment**. The level time is the CPU the job has used at its current
  level. The allotment is 4 ms at Q1 and 8 ms at Q2. The demotion happens at
  the moment the total reaches the allotment, even in the middle of a slice.
  When a yield and the level's allotment fall on the same millisecond, the
  demotion happens. A job at Q3 keeps its level.
- The allotment counts every run at that level. A job that gives up the CPU
  early still spends its allotment.
- A new job enters at the tail of Q1. Jobs that arrive in the same millisecond
  enter in the order the case lists them. A job that arrives now can run now.
- A job that stops running returns to the tail of its own queue. It starts a
  fresh slice when it runs again. The CPU it has used at the level stays on its
  level time.
- A job that changes queue starts its level time at zero.
- The scheduler stops a running job before the end of a slice in four cases.
  Another job arrives in a strictly higher queue, the job reaches its
  allotment, the job yields, or a boost occurs. A new arrival in the same queue
  waits its turn.
- A boost moves every unfinished job to Q1. The boost period is 8 ms in the
  `boost-on` case. A job that arrives in the same millisecond as a boost enters
  Q1 before the boost. The job that was running joins the head of its own queue
  before the merge, so it stays ahead of the other jobs at its level. The queues
  merge in the order Q1, then Q2, then Q3. Each queue keeps its own order. Every
  boosted job starts a fresh slice and a fresh allotment.
- Switching costs no time. A millisecond with no ready job is idle.

## The Cases

The simulator runs the five cases below. It takes no other workload.

Each command below prints the text under it. The text is the expected output.

### `learning`

The run has a long job and two short jobs. The short jobs arrive later, while the
long job sinks to Q2 and then to Q3.

    ./mlfq --case learning

```
case: learning
model: queues 3, slice 2/4/8 ms, allotment 4/8 ms, boost off, accounting allotment, no switch cost
jobs: A(arr 0 cpu 20) B(arr 5 cpu 3) C(arr 6 cpu 3)

schedule
  0-4      A    Q1
  4-5      A    Q2
  5-7      B    Q1
  7-9      C    Q1
  9-10     B    Q1
  10-11    C    Q1
  11-18    A    Q2
  18-26    A    Q3

metrics
job     arr  finish  turnaround  response
A         0      26          26         0
B         5      10           5         0
C         6      11           5         1
average                    12.00      0.33
```

### `gaming-old`

One job gives up the CPU after every millisecond. The other job has no such
habit. The demotion rule is the early one, which demotes a job only when the
job uses a whole slice in one run.

    ./mlfq --case gaming-old

```
case: gaming-old
model: queues 3, slice 2/4/8 ms, allotment 4/8 ms, boost off, accounting slice, no switch cost
jobs: G(arr 0 cpu 20 yield 1) H(arr 0 cpu 4)

schedule
  0-1      G    Q1
  1-3      H    Q1
  3-22     G    Q1
  22-24    H    Q2

metrics
job     arr  finish  turnaround  response
G         0      22          22         0
H         0      24          24         1
average                    23.00      0.50
```

The yielding job holds Q1 from 3 ms to 22 ms. It never uses a whole slice, so
the early rule never demotes it. The other job waits in Q2 until the yielding
job finishes.

### `gaming-new`

This run uses the same two jobs. The demotion rule is the settled one, which
counts every run at a level.

    ./mlfq --case gaming-new

```
case: gaming-new
model: queues 3, slice 2/4/8 ms, allotment 4/8 ms, boost off, accounting allotment, no switch cost
jobs: G(arr 0 cpu 20 yield 1) H(arr 0 cpu 4)

schedule
  0-1      G    Q1
  1-3      H    Q1
  3-4      G    Q1
  4-6      H    Q1
  6-8      G    Q1
  8-16     G    Q2
  16-24    G    Q3

metrics
job     arr  finish  turnaround  response
G         0      24          24         0
H         0       6           6         1
average                    15.00      0.50
```

The yielding job spends its 4 ms at Q1, so the settled rule demotes it to Q2
at 8 ms and then to Q3 at 16 ms. The other job now finishes at 6 ms instead of
24 ms.

### `boost-off`

The run has a long job and three short jobs. The short jobs arrive at 4 ms,
8 ms, and 12 ms. There is no boost.

    ./mlfq --case boost-off

```
case: boost-off
model: queues 3, slice 2/4/8 ms, allotment 4/8 ms, boost off, accounting allotment, no switch cost
jobs: L(arr 0 cpu 8) s1(arr 4 cpu 4) s2(arr 8 cpu 4) s3(arr 12 cpu 4)

schedule
  0-4      L    Q1
  4-8      s1   Q1
  8-12     s2   Q1
  12-16    s3   Q1
  16-20    L    Q2

metrics
job     arr  finish  turnaround  response
L         0      20          20         0
s1        4       8           4         0
s2        8      12           4         0
s3       12      16           4         0
average                     8.00      0.00
```

The long job runs for 4 ms, sinks to Q2 at 4 ms, and then gets no CPU for
12 ms. The three short jobs hold Q1 from 4 ms to 16 ms.

### `boost-on`

This run uses the same four jobs. The boost period is 8 ms. The boosts occur at
8 ms and 16 ms. The run ends at 20 ms, before the next one at 24 ms.

    ./mlfq --case boost-on

```
case: boost-on
model: queues 3, slice 2/4/8 ms, allotment 4/8 ms, boost every 8 ms, accounting allotment, no switch cost
jobs: L(arr 0 cpu 8) s1(arr 4 cpu 4) s2(arr 8 cpu 4) s3(arr 12 cpu 4)

schedule
  0-4      L    Q1
  4-8      s1   Q1
  8-10     s2   Q1
  10-12    L    Q1
  12-14    s2   Q1
  14-16    L    Q1
  16-20    s3   Q1

metrics
job     arr  finish  turnaround  response
L         0      16          16         0
s1        4       8           4         0
s2        8      14           6         0
s3       12      20           8         4
average                     8.50      1.00
```

The boost at 8 ms returns the long job to Q1. The long job now runs at 10 ms
instead of 16 ms. It finishes at 16 ms instead of 20 ms. The last short job
pays for that. It finishes at 20 ms instead of 16 ms.

## Files

| File | What it is |
|---|---|
| `mlfq.c` | The simulator |
| `expected/<case>.txt` | The exact output that `make check` compares against |
| `Makefile` | The build and check rules |
| `README.md` | This file |
