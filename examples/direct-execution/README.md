<!--no-pdf-->
# Direct Execution, Traps, and Preemption Examples

These three programs show three kernel ideas at user level: the trap, the cost
of a trap, and a timer that interrupts a running loop. They're analogies. They don't
implement the kernel.

## Build and Check

Run these commands in this directory.

Build every example:

```bash
make
```

Build every example. Run it. Compare the output:

```bash
make check
```

`make check` exits 0 only when every example passes. It prints one line per
example, then a count. The count is `3 examples, 3 passed, 0 failed` on success.

Remove the built programs:

```bash
make clean
```

The Makefile compiles each example with:

```bash
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L
```

## The Examples

| File | Command | Expected result |
|:--|:--|:--|
| `raw_syscall.c` | `./raw_syscall` | Four exact lines. The library call and the raw system call each return 25 bytes. |
| `trap_cost.c` | `./trap_cost` | Two numbers. The trap costs more per call than the plain call. |
| `timer_interrupt.c` | `./timer_interrupt` | One exact line, `timer fired 20 times`, then the loop count. |

### `raw_syscall`

This program prints one message twice, both times through a library call. The
first is `write(2)`, the library's own wrapper for the write service. The second
is `syscall(2)`, the library's generic wrapper, which takes the kernel's
number for the write service as its first argument. Both wrappers prepare the
same kernel interface and run the same trap. Both calls return the same count.

```bash
./raw_syscall
```

Output:

```
limited direct execution
limited direct execution
libc write returned 25
syscall write returned 25
```

The two `write` calls are unbuffered. They reach the output first. The two
`printf` lines are buffered. They flush at exit. This order holds for a terminal
and for a pipe.

`syscall(2)` is a Linux interface. POSIX doesn't define it. The file defines
`_DEFAULT_SOURCE` to expose the call under the build above.

### `trap_cost`

This program times a plain function call against `syscall(SYS_getpid)`.
`plain_target()` is a local function, so the kernel takes no part. The system
call crosses into the kernel and returns. The measured system-call time is larger.

```bash
./trap_cost
```

Output on the test machine:

```
plain_call_ns=1.834
trap_call_ns=205.333
```

The numbers change with the machine and with the load. The test checks a
property. The trap must cost more per call than the plain call does.

### `timer_interrupt`

This program asks the kernel for an interval timer with `setitimer(2)`. The
kernel raises `SIGALRM` every millisecond. A handler counts the ticks. The main
loop asks the kernel for nothing and never yields. The loop still loses the CPU
for each tick, and control returns to the loop afterwards. After twenty ticks the
loop stops on its own.

```bash
./timer_interrupt
```

Output on the test machine:

```
timer fired 20 times
loop iterations: 57752993
```

The first line is exact. The iteration count changes with the machine. The test
checks that the count is a positive integer, which shows that the loop resumed
after each tick.

## Where to Build and Run

Build these inside Ubuntu. Run them there, on any of the three hosts:

| Host | The Ubuntu you use |
|:--|:--|
| Windows | WSL2, from b1 |
| macOS | The Ubuntu 24.04 Server virtual machine from b1 Part 4, run under UTM |
| Linux | Your own kernel |

The `syscall(2)` interface is Linux-only, so it stays inside that Ubuntu. Don't
try these on native macOS.

## Machine Used for These Runs

```
cc (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0
Linux 6.6.87.2-microsoft-standard-WSL2 x86_64
GNU Make 4.3
```
