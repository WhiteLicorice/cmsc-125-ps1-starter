<!--no-pdf-->
# Activity 6 Examples

`sim.c` is a round-robin scheduler simulator. It runs one workload and prints
the schedule, the finish time, the turnaround time, the response time, and the
averages for every job.

The simulator is deterministic. One case gives one answer. The program reads no
input and uses no random numbers.

## Requirements

- A C compiler. The outputs below come from `cc` in GCC 13.3.0.
- `make`.

## Build and Run

Run these commands in this directory.

Build the simulator:

    make

Run one case:

    ./sim equal-q1

Run every case:

    ./sim

Build the simulator. Run every case. Compare the output:

    make check

Delete the built simulator:

    make clean

`make check` prints one line for each case, then a count line. It exits 0 when
every case matches. The expected output for each case is in `expected/`.

The Makefile compiles the simulator with:

    cc -std=c11 -Wall -Wextra -Werror

## Where to Build and Run

Build these in Ubuntu. Run them there, on any of the three hosts:

| Host | The Ubuntu you use |
|:--|:--|
| Windows | WSL2, from `b1` |
| macOS | The Ubuntu 24.04 Server virtual machine from `b1` Part 4, run under UTM |
| Linux | Your own kernel |

The programs are POSIX C. They build on native macOS, but the course documents
one host per platform, so use the Ubuntu guest.

## The Model

The simulator follows one model. Every case below uses it.

- One tick is one time unit. The quantum counts in ticks.
- The ready queue is first-in first-out.
- A job that arrives, or that finishes its I/O, enters the tail of the ready
  queue. A preempted job enters the tail after that job.
- A job that uses its full quantum leaves the CPU. A job that finishes its CPU
  work leaves the CPU, then blocks for I/O or finishes.
- A context switch costs no ticks. The `switch-cost` case reports the cost on
  its own.

The simulator prints `idle` when no job is ready.

## The Cases

| Case | Command | Expected result |
|:--|:--|:--|
| `equal-q1` | `./sim equal-q1` | Three jobs of 5 ticks each. Average turnaround 14.00, average response 1.00. |
| `equal-q5` | `./sim equal-q5` | The same jobs with a quantum of 5. Average turnaround 10.00, average response 5.00. |
| `mixed-q5` | `./sim mixed-q5` | Jobs of 120, 60, and 30 ticks. Average turnaround 150.00, average response 5.00. |
| `arrivals-q20` | `./sim arrivals-q20` | Four jobs that arrive one after another. Average turnaround 67.50, average response 20.00. |
| `io-alone` | `./sim io-alone` | One job that blocks for I/O. CPU utilization 50.0 percent. |
| `switch-cost` | `./sim switch-cost` | The switch cost for one workload at four quanta. |

## The Program

| File | What it is |
|:--|:--|
| `sim.c` | The worked-example cases |
| `sim_engine.c` | The scheduler engine |
| `sim_engine.h` | The engine interface |
| `expected/<case>.txt` | The exact output that `make check` compares against |
| `Makefile` | The build and check rules |
| `README.md` | This file |

## Machine Used for These Runs

    cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
    Linux 6.6.87.2-microsoft-standard-WSL2 x86_64
    GNU Make 4.3
