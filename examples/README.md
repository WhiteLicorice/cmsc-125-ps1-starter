<!--no-pdf-->
# Activity Examples

This folder holds the worked examples for the activities that precede Problem
Set 1, Processes and Preemptive Scheduling. Each activity has its own folder.
Each folder holds its own `make` and `make check`.

These examples are not graded. They are not one of the stages of the problem
set. Use them to see the mechanism before you write the code.

| Folder | Activity |
|:--|:--|
| `act2` | One Program Becomes Two |
| `act3` | Direct Execution, Traps, and Preemption |
| `act4` | Wiring Processes Together |
| `act5` | Scheduling Metrics, FIFO, SJF, and STCF |
| `act6` | Round-Robin, Response Time, and the Time Slice |
| `act7` | The Multilevel Feedback Queue |

Build and check one folder:

```bash
cd examples/act2
make
make check
make clean
```

`make` builds the examples. `make check` builds them and compares the output
with the recorded output. `make clean` removes the build output. Read the
`README.md` in each folder for the model and the cases.

Run these commands inside Ubuntu. The examples need Linux.
