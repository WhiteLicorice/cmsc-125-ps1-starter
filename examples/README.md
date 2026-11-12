# Worked Examples

This folder holds worked examples for four subtopics in Unit 1. Each folder is
a runnable program with its own `make` and `make check`.

These examples aren't graded. They aren't part of the work you submit. Use
them to see the mechanism before you write the code.

| Folder | Subtopic |
|:--|:--|
| `process-api` | "Processes and the Process API" |
| `direct-execution` | "Direct Execution, Traps, and Preemption" |
| `file-descriptors` | "File Descriptors, Pipes, and Process Control" |
| `cpu-scheduling-metrics` | "CPU Scheduling and Multilevel Feedback Queues", response time and the three policies |
| `cpu-scheduling-round-robin` | "CPU Scheduling and Multilevel Feedback Queues", the quantum and taking turns |
| `cpu-scheduling-mlfq` | "CPU Scheduling and Multilevel Feedback Queues", the multilevel feedback queue |

Three folders share one subtopic because that subtopic carries three separate
models. `cpu-scheduling-metrics` comes first, since it defines response time,
turnaround time, and the policies the other two build on. Read its `README.md`
before either of them.

Build and check one folder:

```bash
cd examples/process-api
make
make check
make clean
```

`make` builds the examples. `make check` builds them and compares the output
with the recorded output. `make clean` removes the build output. Read the
`README.md` in each folder for the model and the cases.

Run these commands inside Ubuntu. The examples need Linux.
