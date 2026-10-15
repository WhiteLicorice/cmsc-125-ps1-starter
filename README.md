<!--no-pdf-->
# CMSC 125 Problem Set 1 Starter

This C17 project is the starter for the CMSC 125 Problem Set 1 controller. The
controller forks worker processes, decides which worker runs, and preempts the
running one with `SIGSTOP`. Each worker reports its ticks on a pipe and keeps
computing until the stop lands, so a slice runs a tick or two past its quantum.
This README gives the repository map, the commands, the starting state, and the
three host routes.

This project runs on Linux, on macOS, and on Windows through WSL2 Ubuntu. The
controller uses POSIX calls only.

## Pair

Replace the two entries below. An assigned trio adds one entry.

- Full Name (`@github-username`)
- Full Name (`@github-username`)

## Files You May Change

You may change only these files and folders:

- `src/workload.c`
- `src/controller.c`
- `src/worker.c`
- `src/report.c`
- `ANALYSIS.md`, which you create
- The Pair section of `README.md`
- `student-cases/`, which holds the one case of your own

Everything else is fixed. `include/ps1.h`, `src/main.c`, `src/clock.c`, and
`src/trace.c` are given to you complete. `cases/`, `tools/check_cases.py`, and
the workflow are the grading corpus and its runner. The manual states every
rule these files follow.

## Layout

```
include/ps1.h       fixed interface: limits, codes, structures, prototypes
src/main.c          complete: the command line and the exit codes
src/clock.c         complete: the CLOCK_MONOTONIC reading
src/trace.c         complete: the per-worker trace writer
src/workload.c      yours: read and validate a workload file
src/controller.c    yours: fork, dispatch, preempt, reap
src/worker.c        yours: the CPU-bound child
src/report.c        yours: response time, turnaround time, averages
cases/              the complete public corpus
student-cases/      the one boundary or failure case of your own
tools/check_cases.py  the runner that grades the corpus
ANALYSIS.md         the joint analysis that you create
Makefile run check.sh  the run contract
```

## Commands

```bash
make                # build build/cmsc-125-ps1
make clean          # remove the build directory
./run --policy rr --quantum 5 cases/normal/rr_two.work
./check.sh          # build, then run every published case in cases/
./check.sh student-cases  # build, then run the cases in student-cases/
```

`./run` passes its arguments to the binary. `--trace-dir DIR` writes the trace
files to `DIR` and defaults to `traces`.

`check.sh` is the complete public automated check. No hidden suite exists. Read
`tools/check_cases.py` before you trust a green run. The rubric separately
assesses the joint analysis, the collaboration history, and the traces you
submit.

## Exit Codes

| Code | Meaning |
|---|---|
| 0 | Every worker finished its burst. |
| 65 | The command line or the workload file was refused. |
| 70 | A worker failed before its burst was done, or a system call did. |
| 130 | `SIGINT` ended the run. The report is partial. |

## The First Run

A fresh clone builds without warnings. It passes 8 of the 57 cases and exits 1.
That number needs care, because the 8 passing cases test nothing you wrote.
They are the command-line refusals. `src/main.c` is given complete, so a bad
option is refused before your code runs at all.

The other 49 cases need `src/workload.c`, `src/controller.c`, `src/worker.c`,
and `src/report.c`. The workflow in the Actions tab is red for the same reason.
That state is correct for a starter. Your fork passes after you implement the
four files.

On the fork, open the Actions tab once and enable workflows. GitHub disables
them on every fork, and the workflow run is part of the submission.

## Hosts

### Linux

```bash
sudo apt update
sudo apt install -y build-essential make python3
```

### macOS

The Xcode command line tools carry the compiler and `make`.

```bash
xcode-select --install
```

Check the Python 3 version. macOS ships Python 3 with the command line tools.

```bash
python3 --version
```

### Windows through WSL2 Ubuntu

Install WSL2 with Ubuntu from PowerShell.

```powershell
wsl --install -d Ubuntu
```

Open the Ubuntu terminal and install the tools inside it.

```bash
sudo apt update
sudo apt install -y build-essential make python3
```

Work inside the Ubuntu file system, which is your home directory. A checkout
under `/mnt/c` works but it is slower, and file permissions there behave
differently.

### Check

```bash
cc --version
make --version
python3 --version
```

## The Corpus

The 57 cases under `cases/` are the cases the instructor grades with. Each
`.work` file holds a workload. Each `.args` file holds the command line. Each
`.expect` file holds the expected exit code, the expected diagnostic, and the
paper schedule: the dispatch order the policy gives with no overrun.

The one case your pair writes lives under `student-cases/`. That folder holds a
short README and an example in the same three-file form. Run it with
`./check.sh student-cases`.

The runner replays each run from its traces and checks the decisions the policy
made: a round-robin slice ends at exactly `quantum` bytes, and a stopped worker
never gains a tick before its next dispatch. Every tick count and average in the
report is exact against the replay of that run's own traces. A measured time is
compared against a stated window, never against an exact value, because a real
tick costs what the machine decides. The overrun moves the ticks from run to
run, so the runner does not compare a run with one fixed schedule. The manual
states the window, the overrun, and the reason for both.

