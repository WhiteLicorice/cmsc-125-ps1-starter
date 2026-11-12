<!--no-pdf-->
# File Descriptors, Pipes, and Process Control Examples

These programs show the descriptor table, redirection, pipes, and process
control. Each program is one C file with its own `main`.

The programs run on Linux. On Windows, run them inside Ubuntu under Windows
Subsystem for Linux (WSL2). On macOS, run them inside the Ubuntu 24.04 Server
virtual machine from the `b1` manual, Part 4, run with UTM. On Linux, run them
in your own terminal. Run every command inside the Ubuntu guest, on Windows or
on a Mac. `zombie.c` reads `/proc`, so it runs in the guest on a Mac too.

## Requirements

- A C compiler. The outputs below come from `cc` in GCC 13.3.0.
- `make`.

## Build and Run

Build one example. Run the example. Replace `fd_table` with the file name of any example.

    cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -o fd_table fd_table.c
    ./fd_table

Build every example.

    make

Build every example. Run each example. Compare each output with the expected output.

    make check

Delete the binaries and the files the examples write.

    make clean

`make check` prints one line for each example, and then a count line. It exits
0 when every example passes. It exits non-zero when an example fails or exceeds the time limit.

## How the Check Decides

`make check` merges the standard output and the standard error of each example.
It compares the result with `expected/<name>.txt`. Every example is
deterministic, so every comparison is exact. Each run stops after ten seconds.

Each example sets its standard output to unbuffered. The order you see is the
order the writes reached the pipe.

Each example reaps every child it forks. A grandchild appears in two of the examples. The
parent doesn't fork that grandchild. The kernel reaps the grandchild after it
exits, and `make check` waits for the last writer of the output pipe to close.

The five examples that fork run a `wait` before they print their summary line.
The summary line is therefore always the last line of output.

## The Examples

### `fd_table.c`

This example shows which number the descriptor table returns.

    ./fd_table

```
descriptor 0 is open at start
descriptor 1 is open at start
descriptor 2 is open at start
first open returned 3
open after close returned 3
open after close(1) returned 1
stdout restored
```

The first three lines state the property that the rest depends on. If your
shell starts the program with another descriptor open, the numbers shift and
the last three lines change.

### `redirect.c`

Point descriptor 1 at a file.

    ./redirect

```
terminal: before the redirect
redirect.out holds: file: after the redirect
```

The file `redirect.out` holds one line after the run:

```
file: after the redirect
```

### `pipe_basic.c`

This example creates a pipe between a parent and a child.

    ./pipe_basic

```
parent: pipe read end is 3, write end is 4
child: read 22 bytes
child: body "read,read|write,write|"
child: read returned 0 after every write end closed
parent: reaped the child, exit status 0
parent: child pid matched: yes
```

The child replaces each newline with `|`, so the payload stays on one line. The
child prints before the parent, which prints after `waitpid` returns.

### `pipe_eof.c`

This example keeps a write end open in another process.

    ./pipe_eof

```
parent: read 8 bytes "one,one|"
parent: read 8 more bytes "two,two|"
parent: read after the holder closes returns 0
parent: reaped the middle child, exit status 0
```

After the reader drains the first payload, it sends the stop byte. The holder
then writes the second payload. The reader's next read returns eight more bytes, the
second payload. A read that returned 0 there would mean the pipe was finished,
with no writer left. The pipe wasn't finished. The last read returns 0 only
after the holder closes, when every writer has closed.

### `zombie.c`

This example shows a child that exits before its parent collects its exit status.

    ./zombie

```
parent: /proc state before wait is Z
parent: waitpid reaped the pid it forked: yes
parent: WIFEXITED 1, WEXITSTATUS 7
parent: /proc state after wait is gone
```

This example reads `/proc/<pid>/stat`. The state field is the third field of
that file. After the closing parenthesis of the command name,
the example reads the character after the next space.

### `orphan.c`

This example shows a process that outlives its parent.

    ./orphan

```
parent: reaped the middle child, exit status 0
parent: the grandchild observed a new parent pid: yes
```

The middle child waits on a pipe until the grandchild reports that it has read
its own parent pid. Only then does the middle child exit. Without that
handshake, the middle child can exit first, the grandchild reads the new pid as
its original one, and the example reports no change.

### `signal_kill.c`

Send a signal to a child. Read the outcome.

    ./signal_kill

```
parent: child announced it is about to pause
parent: sent SIGTERM
parent: stopped by a signal: yes
parent: terminating signal number: 15
parent: SIGTERM number: 15
```

The child writes one byte on a pipe before it blocks. The parent reads that
byte first, which proves the child reached the `write`, not that it reached
`pause`. `SIGTERM` ends the child either way, because termination is the
default action for that signal.

## Files

| File | What it is |
|---|---|
| `<name>.c` | The source of one example |
| `expected/<name>.txt` | The exact output that `make check` compares against |
| `Makefile` | The build and check rules |
| `README.md` | This file |
