<!--no-pdf-->
# Processes and the Process API Examples

This folder holds one C program per example in "Processes and the Process API" in Unit 1.
Each program shows one fact about `fork`. Run the checks on Linux. On Windows,
run them inside Ubuntu under WSL2. On macOS, run them inside the Ubuntu virtual
machine from Part 4 of the b1 bootcamp, which uses UTM and Ubuntu 24.04 Server.
Build every program inside that guest. Run every command inside that guest.

## Build and Check

```bash
make check
```

`make check` compiles each program with
`cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L`. It runs each
program and compares the output with the expected output. It prints one line
per program, one line for the leftover-process check, and a count of the
programs. It exits 0 only when every check passes. The build stays clean under
`-Werror`.

Run `make clean` to remove the programs and the temporary comparison files.

The buffering check needs `script` from util-linux. It runs that program once
on a terminal and twice with the output redirected.

## The Programs

| Program | Command | Expected output or property |
|---|---|---|
| `fork_basic.c` | `./fork_basic` | Two lines. Either line may come first. The child line reports `fork 0`. The parent line reports the child's pid, and the two pids differ. |
| `fork_wait.c` | `./fork_wait` | Two lines, with the child line first. The parent line reports the pid that `wait` returned, and that pid is the child's. |
| `fork_copy.c` | `./fork_copy` | Three lines, in this order: `child: x = 400`, `parent: x = 100`, `parent: x = 300 after the change`. |
| `fork_exec.c` | `./fork_exec` | Two lines, in this order: `the child is now echo`, `parent: the child finished`. |
| `shell_loop.c` | `printf 'echo one\necho two\n' \| ./shell_loop` | Two lines, in this order: `one`, `two`. |
| `buffering.c` | `./buffering > out.txt` | Four lines, in this order: `before fork`, `child`, `before fork`, `parent`. |

The pid values change on every run. Only the relation between them is fixed.

## The Buffering Program

`buffering.c` prints one line before it calls `fork`. The number of lines
depends on where the standard output goes.

| Command | The lines it prints |
|---|---|
| `script -qc ./buffering /dev/null` | `before fork`, `child`, `parent` |
| `./buffering > out.txt` | `before fork`, `child`, `before fork`, `parent` |
| `./buffering flush > out.txt` | `before fork`, `child`, `parent` |

Standard output is line buffered on a terminal. The line leaves the buffer at
the newline, so the child receives an empty buffer. Standard output is fully
buffered on a pipe or a file. The line waits in the buffer, `fork` copies the
whole process image along with that buffer, and both copies write the line
at exit.

The word `flush` forces the program to write the line before `fork` returns. Both modes print
three lines on a terminal.

## The Exit Status of a Check

`make check` reports a failure in two places. The first is the line for the
program. The second is the exit status of `make`. A broken expectation for
`fork_copy.c` prints this line and exits 2.

```
FAIL fork_copy: the parent did not keep its own copy of x
```
