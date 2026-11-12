<!--no-pdf-->
# CPU Scheduling and Multilevel Feedback Queues Examples

`schedsim.c` is a deterministic CPU-scheduling simulator. `demo_cases.c` holds
one list of cases. One case is one scheduling policy on one workload. The
program prints the schedule, a Gantt row, and the per-job metrics. The recorded
output of every case is in `expected/`. Every case runs on the same model and a
whole-number clock, so the same input always produces the same output on every
host.

This folder holds the seven worked-example cases only.

## The Model

The simulator uses one fixed model. Every case below exercises that model.

- The time unit is one tick. Every arrival time and every burst time is a whole
  number of ticks.
- A job holds a label, an arrival time, and a burst time.
- At time `t`, every job whose arrival time is `t` joins the ready jobs. The
  scheduler then chooses a job.
- FIFO chooses the ready job that arrived first. A tie goes to the job listed
  first.
- SJF chooses the ready job with the smallest burst time. A tie goes to the job
  listed first.
- STCF chooses the ready job with the smallest remaining time. A tie goes to the
  job listed first, unless the running job is in the tie. The running job then
  continues.
- FIFO and SJF are non-preemptive. A job that starts runs to completion.
- STCF is preemptive. It rechecks the choice at every arrival.
- A switch costs no time. The model leaves out the context-switch cost.
- Completion time is the time the job finishes.
- Turnaround time is the completion time minus the arrival time.
- Response time is the first run time minus the arrival time.

## Build and Check

Run these commands in this directory.

Build the simulator:

```bash
make
```

Build every case. Run it. Compare the output:

```bash
make check
```

`make check` exits 0 only when every case passes. It prints one line per case,
then a count. The count is `7 cases, 7 passed, 0 failed` on success.

Remove the built program:

```bash
make clean
```

The Makefile compiles the simulator with:

```bash
cc -std=c11 -Wall -Wextra -Werror
```

## The Cases

The workload column reads `name:arrival/burst`.

| Case | Command | Workload | Run order | Average turnaround | Average response |
|:--|:--|:--|:--|--:|--:|
| `fifo-basic` | `./schedsim fifo-basic` | `A:0/10 B:0/10 C:0/10` | A B C | 20.00 | 10.00 |
| `fifo-convoy` | `./schedsim fifo-convoy` | `A:0/120 B:0/60 C:0/30` | A B C | 170.00 | 100.00 |
| `sjf-convoy` | `./schedsim sjf-convoy` | `A:0/120 B:0/60 C:0/30` | C B A | 110.00 | 40.00 |
| `fifo-arrivals` | `./schedsim fifo-arrivals` | `A:0/120 B:10/60 C:10/30` | A B C | 163.33 | 93.33 |
| `sjf-arrivals` | `./schedsim sjf-arrivals` | `A:0/120 B:10/60 C:10/30` | A C B | 153.33 | 83.33 |
| `stcf-arrivals` | `./schedsim stcf-arrivals` | `A:0/120 B:10/60 C:10/30` | A C B A | 110.00 | 10.00 |
| `stcf-same` | `./schedsim stcf-same` | `A:0/120 B:0/60 C:0/30` | C B A | 110.00 | 40.00 |

Run the simulator with no argument to print every case at once.

## Recorded Output

Each block below is the exact text in `expected/<case>.txt`.

### `fifo-basic`

```
case fifo-basic
policy FIFO
workload A:0/10 B:0/10 C:0/10

schedule
  0-10 A
  10-20 B
  20-30 C

gantt (1 column = 1 time unit)
  AAAAAAAAAABBBBBBBBBBCCCCCCCCCC

metrics
  job arrival burst completion turnaround response
  A   0       10    10         10         0
  B   0       10    20         20         10
  C   0       10    30         30         20
  avg                          20.00      10.00
```

### `fifo-convoy`

```
case fifo-convoy
policy FIFO
workload A:0/120 B:0/60 C:0/30

schedule
  0-120 A
  120-180 B
  180-210 C

gantt (1 column = 10 time units)
  AAAAAAAAAAAABBBBBBCCC

metrics
  job arrival burst completion turnaround response
  A   0       120   120        120        0
  B   0       60    180        180        120
  C   0       30    210        210        180
  avg                          170.00     100.00
```

### `sjf-convoy`

```
case sjf-convoy
policy SJF
workload A:0/120 B:0/60 C:0/30

schedule
  0-30 C
  30-90 B
  90-210 A

gantt (1 column = 10 time units)
  CCCBBBBBBAAAAAAAAAAAA

metrics
  job arrival burst completion turnaround response
  A   0       120   210        210        90
  B   0       60    90         90         30
  C   0       30    30         30         0
  avg                          110.00     40.00
```

### `fifo-arrivals`

```
case fifo-arrivals
policy FIFO
workload A:0/120 B:10/60 C:10/30

schedule
  0-120 A
  120-180 B
  180-210 C

gantt (1 column = 10 time units)
  AAAAAAAAAAAABBBBBBCCC

metrics
  job arrival burst completion turnaround response
  A   0       120   120        120        0
  B   10      60    180        170        110
  C   10      30    210        200        170
  avg                          163.33     93.33
```

### `sjf-arrivals`

```
case sjf-arrivals
policy SJF
workload A:0/120 B:10/60 C:10/30

schedule
  0-120 A
  120-150 C
  150-210 B

gantt (1 column = 10 time units)
  AAAAAAAAAAAACCCBBBBBB

metrics
  job arrival burst completion turnaround response
  A   0       120   120        120        0
  B   10      60    210        200        140
  C   10      30    150        140        110
  avg                          153.33     83.33
```

### `stcf-arrivals`

```
case stcf-arrivals
policy STCF
workload A:0/120 B:10/60 C:10/30

schedule
  0-10 A
  10-40 C
  40-100 B
  100-210 A

gantt (1 column = 10 time units)
  ACCCBBBBBBAAAAAAAAAAA

metrics
  job arrival burst completion turnaround response
  A   0       120   210        210        0
  B   10      60    100        90         30
  C   10      30    40         30         0
  avg                          110.00     10.00
```

### `stcf-same`

```
case stcf-same
policy STCF
workload A:0/120 B:0/60 C:0/30

schedule
  0-30 C
  30-90 B
  90-210 A

gantt (1 column = 10 time units)
  CCCBBBBBBAAAAAAAAAAAA

metrics
  job arrival burst completion turnaround response
  A   0       120   210        210        90
  B   0       60    90         90         30
  C   0       30    30         30         0
  avg                          110.00     40.00
```

## Where to Build and Run

Build these inside Ubuntu. Run them there, on any of the three hosts:

| Host | The Ubuntu you use |
|:--|:--|
| Windows | WSL2, from b1 |
| macOS | The Ubuntu 24.04 Server virtual machine from b1 Part 4, run under UTM |
| Linux | Your own kernel |

`schedsim.c` uses the C standard library only. It builds and runs on native
macOS and on native Linux too.

## Machine Used for These Runs

```
cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Linux 6.6.87.2-microsoft-standard-WSL2 x86_64
GNU Make 4.3
```
