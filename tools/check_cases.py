#!/usr/bin/env python3
"""
check_cases.py: the CMSC 125 Problem Set 1 public case runner.

The runner runs the student's own ./run entry point on each .work file under
the case directory. Each case specifies its command line in a matching .args file
and its expectations in a matching .expect file.

An .expect file holds one directive on each line:

    exit <code>          the exit code the run must return
    stderr <text>        a substring stderr must hold (repeatable)
    schedule <w>@<tick>  the paper schedule: the dispatch order for zero overrun
    interrupt SIGINT     send SIGINT to the controller after a short delay
    partial              accept a partial report, as SIGINT and a fault produce
    path <relative>      pass this workload path in place of the case file

The runner does not compare the run against one fixed schedule. A worker keeps
computing until its stop lands, so a slice runs a few ticks past the quantum and
the ticks shift from run to run. The runner instead replays each run from its
traces and checks the decisions:

  * Every dispatch follows the policy's choice. The choice uses queue order,
    arrivals, and each worker's remaining work after any overrun.
  * Only one worker runs at a time. A dispatch cannot occur while another
    worker runs. A preempt, stop, or completion must come from the running worker.
  * The sum of the workers' done counts equals the work clock at every event. Each clock
    tick belongs to exactly one worker.
  * A round-robin preemption occurs at `quantum` bytes into its slice.
  * FIFO never preempts. STCF preempts only when the arriving job is shorter
    than the running worker's remaining work.
  * No worker gains a tick between its `stopped` event and next `dispatch`.
  * Each worker completes its burst exactly. The final clock equals the sum of
    the finished workers' bursts.
  * The report's tick columns and averages match the replay exactly.
  * Each measured time matches its trace within the stated tolerance. A
    finished run reports a response and turnaround time for every worker.
  * Each measured rate exceeds the window's floor. A rate above its ceiling
    reports host load. That result keeps the case valid.

The `.expect` schedule is the paper schedule for zero overrun. A run that
differs from it is reported as information, never as a failure, because the
overrun shifts the later ticks and can shorten a worker's last slice.

Exit code: 0 when every case passes, 1 otherwise.
"""

import argparse
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

# A measured tick costs between these two bounds in milliseconds. One tick is
# PS1_TICK_ITERATIONS dependent multiplies. No machine runs 200,000 dependent
# multiplies in under a microsecond, and a machine that takes a hundred
# milliseconds for them is doing something else. The window is wide on purpose:
# the machine, the compiler, and the load all move the real cost, and gcc and
# clang differ by a factor of three on the same machine.
MS_PER_TICK_MIN = 0.001
MS_PER_TICK_MAX = 100.0

# The runner compares the reported time with the trace value. It allows this
# many milliseconds of difference. Both values come from one run. They should
# agree within one microsecond. An implementation may read the clock twice, once
# for the trace and once for the report. The tolerance accounts for those reads.
# It catches a fabricated response or turnaround when the true value differs by
# more than one millisecond.
MS_TOLERANCE = 1.0

TIMEOUT_SECONDS = 30
INTERRUPT_DELAY_MS = 50
INTERRUPT_SETTLE_MS = 20
TRACE_WAIT_SECONDS = 5.0
PROCESS_NAME = "cmsc-125-ps1"

HEADER_RE = re.compile(
    r"^policy (fifo|rr|stcf) quantum (\d+) workers (\d+) ticks (\d+)$"
)
WORKER_RE = re.compile(
    r"^worker (\S+) arrival (\d+) burst (\d+) "
    r"response_ticks (-|\d+) turnaround_ticks (-|\d+) "
    r"response_ms (-|\d+\.\d+) turnaround_ms (-|\d+\.\d+) "
    r"dispatches (\d+) preemptions (\d+)$"
)
AVERAGE_RE = re.compile(
    r"^average response_ticks (-|\d+\.\d+) turnaround_ticks (-|\d+\.\d+) "
    r"response_ms (-|\d+\.\d+) turnaround_ms (-|\d+\.\d+)$"
)
RESULT_RE = re.compile(r"^result (finished|interrupted|faulted)$")
TRACE_HEADER_RE = re.compile(
    r"^# ps1 trace: worker (\S+) arrival (\d+) burst (\d+) time (\d+\.\d+)$"
)

POLICIES = ("fifo", "rr", "stcf")
EVENT_KINDS = ("dispatch", "preempt", "stopped", "complete")
# The order of events that share one work-clock value: the outgoing event of
# the running worker, then its stopped record, then the next dispatch.
EVENT_RANK = {"preempt": 0, "complete": 0, "stopped": 1, "dispatch": 2}


class CaseError(Exception):
    """A problem in a case file, reported as a message rather than a traceback."""


class Worker:
    def __init__(self, name, arrival, burst, crash=0):
        self.name = name
        self.arrival = arrival
        self.burst = burst
        self.crash = crash


class Trace:
    def __init__(self, name, arrival, burst, arrival_time, events):
        self.name = name
        self.arrival = arrival
        self.burst = burst
        self.arrival_time = arrival_time
        self.events = events


# --------------------------------------------------------------------------
# Case files
# --------------------------------------------------------------------------


def parse_work(path):
    """Reads a workload file. The reader treats malformed files as cases."""
    workers = []
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.split("#", 1)[0].strip()
        if not text:
            continue
        fields = text.split()
        if len(fields) < 3:
            workers.append(None)
            continue
        try:
            arrival = int(fields[1])
            burst = int(fields[2])
        except ValueError:
            workers.append(None)
            continue
        crash = 0
        if len(fields) >= 4 and fields[3].startswith("crash="):
            try:
                crash = int(fields[3][len("crash="):])
            except ValueError:
                crash = 0
        workers.append(Worker(fields[0], arrival, burst, crash))
    return workers


def parse_args_file(path):
    """Reads a .args file. Absent means policy fifo."""
    if not path.exists():
        return ["--policy", "fifo"]
    text = path.read_text(encoding="utf-8")
    return text.split("#", 1)[0].split()


def parse_expect(path):
    """Reads an .expect file into a dict of directives."""
    spec = {"exit": 0, "stderr": [], "schedule": None, "interrupt": None,
            "partial": False, "path": None}
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if not text or text.startswith("#"):
            continue
        word, _, rest = text.partition(" ")
        rest = rest.strip()
        if word == "exit":
            spec["exit"] = int(rest)
        elif word == "stderr":
            spec["stderr"].append(rest)
        elif word == "schedule":
            spec["schedule"] = rest.split()
        elif word == "interrupt":
            spec["interrupt"] = rest
        elif word == "partial":
            spec["partial"] = True
        elif word == "path":
            spec["path"] = rest
        else:
            raise CaseError(f"'{path.name}' holds an unknown directive '{word}'.")
    return spec


def policy_of(args):
    if "--policy" in args:
        return args[args.index("--policy") + 1]
    return "fifo"


def quantum_of(args):
    if "--quantum" in args:
        try:
            return int(args[args.index("--quantum") + 1])
        except (ValueError, IndexError):
            return 0
    return 0


# --------------------------------------------------------------------------
# The paper schedule, for zero overrun
# --------------------------------------------------------------------------


def policy_engine(workers, policy, quantum):
    """
    Runs the contract's policies on the tick clock, with no overrun.

    Returns (dispatch pairs, finish ticks). The model follows the manual's
    definitions. Readers can check each implementation against this model. A job whose
    quantum ends joins the tail behind any job that arrived at that same tick,
    which is the ordering the manual states and the controller follows.
    """
    count = len(workers)
    done = [0] * count
    finish = [None] * count
    queue = []
    running = None
    slice_ticks = 0
    tick = 0
    total = sum(w.burst for w in workers)
    dispatches = []
    pending = None

    def remaining(index):
        return workers[index].burst - done[index]

    while tick < total:
        released = []
        for index in range(count):
            if workers[index].arrival == tick and done[index] < workers[index].burst:
                released.append(index)
                queue.append(index)
        if pending is not None:
            queue.append(pending)
            pending = None

        if policy == "stcf" and running is not None and released:
            if any(remaining(i) < remaining(running) for i in released):
                running = None
                slice_ticks = 0

        if running is None:
            if policy == "stcf":
                ready = [i for i in range(count)
                         if workers[i].arrival <= tick and done[i] < workers[i].burst]
                if not ready:
                    raise CaseError("the model found nothing to run (a stall).")
                running = min(ready, key=lambda i: (remaining(i), workers[i].arrival, i))
            else:
                if not queue:
                    raise CaseError("the model found nothing to run (a stall).")
                running = queue.pop(0)
            dispatches.append((workers[running].name, tick))
            slice_ticks = 0

        done[running] += 1
        tick += 1
        slice_ticks += 1

        if done[running] == workers[running].burst:
            finish[running] = tick
            running = None
            slice_ticks = 0
        elif policy == "rr" and slice_ticks == quantum:
            pending = running
            running = None
            slice_ticks = 0

    return dispatches, finish


def expect_from_schedule(schedule):
    """Turns the .expect schedule tokens into dispatch pairs."""
    dispatches = []
    for token in schedule:
        name, _, tick = token.partition("@")
        if not name or not tick.isdigit():
            raise CaseError(f"'{token}' is not a <worker>@<tick> schedule token.")
        dispatches.append((name, int(tick)))
    return dispatches


# --------------------------------------------------------------------------
# Traces
# --------------------------------------------------------------------------


def parse_trace(path):
    """Reads one trace file into a Trace, or raises CaseError on a bad line."""
    header = None
    events = []
    for line in path.read_text(encoding="utf-8").splitlines():
        text = line.strip()
        if not text:
            continue
        if text.startswith("#"):
            match = TRACE_HEADER_RE.match(text)
            if match:
                header = match
            continue
        fields = text.split()
        if len(fields) != 4:
            raise CaseError(f"trace {path.name}: '{text}' does not hold 4 fields")
        kind, tick, done, seconds = fields
        if kind not in EVENT_KINDS:
            raise CaseError(f"trace {path.name}: '{kind}' is not a known event")
        try:
            events.append({"kind": kind, "clock": int(tick), "done": int(done),
                           "seconds": float(seconds)})
        except ValueError:
            raise CaseError(f"trace {path.name}: '{text}' holds a non-numeric field")
    if header is None:
        # A worker that was never released holds an empty file, which happens
        # in a partial run. Its events, if any, still stand.
        return Trace(path.stem, None, None, None, events)
    return Trace(header.group(1), int(header.group(2)), int(header.group(3)),
                 float(header.group(4)), events)


def load_traces(trace_dir, workers, problems):
    """Reads every worker's trace. Returns a dict, or None when one is missing."""
    traces = {}
    for worker in workers:
        path = trace_dir / f"{worker.name}.trace"
        if not path.exists():
            problems.append(f"the trace file '{worker.name}.trace' is missing")
            return None
        try:
            trace = parse_trace(path)
        except CaseError as error:
            problems.append(str(error))
            return None
        if trace.arrival is None:
            problems.append(f"trace {worker.name}.trace: the arrival header is "
                            "missing")
            return None
        traces[worker.name] = trace
    return traces


def load_partial_traces(trace_dir, workers):
    """Reads the traces a partial run left, skipping any it cannot read.

    check_trace_shape reports a missing or malformed trace, so this reader stays
    silent and the row check leaves that worker's counts alone.
    """
    traces = {}
    for worker in workers:
        path = trace_dir / f"{worker.name}.trace"
        if not path.exists():
            continue
        try:
            traces[worker.name] = parse_trace(path)
        except CaseError:
            continue
    return traces


# --------------------------------------------------------------------------
# The replay
# --------------------------------------------------------------------------


def replay(workers, policy, quantum, traces, problems):
    """
    Replays a finished run from its traces and checks the policy decisions.

    Returns the derived metrics, or None when the traces cannot be replayed.
    Each recorded decision is checked against the policy, using the state the
    replay holds at that decision: the released set, the queue order, and each
    worker's remaining work after any overrun.
    """
    by_name = {w.name: w for w in workers}
    order_index = {w.name: i for i, w in enumerate(workers)}

    events = []
    for worker in workers:
        trace = traces[worker.name]
        if trace.arrival != worker.arrival or trace.burst != worker.burst:
            problems.append(
                f"trace {worker.name}: header says arrival {trace.arrival} burst "
                f"{trace.burst}, expected {worker.arrival} and {worker.burst}")
        for position, event in enumerate(trace.events):
            events.append((event["clock"], EVENT_RANK[event["kind"]],
                           order_index[worker.name], position, worker.name, event))
    # The work clock never moves backwards and no two events of one worker
    # repeat a position, so this ordering is the run's own order.
    events.sort(key=lambda item: (item[0], item[1], item[2], item[3]))

    done = {w.name: 0 for w in workers}
    released = {w.name: False for w in workers}
    finished = {w.name: False for w in workers}
    queue = []
    running = None
    last_stopped = {}
    dispatch_clock = {}
    dispatch_done = {}
    preempt_clock = {}
    preempt_done = {}
    preempt_ticks = {w.name: set() for w in workers}
    dispatches = {w.name: 0 for w in workers}
    preemptions = {w.name: 0 for w in workers}
    response_ticks = {w.name: None for w in workers}
    turnaround_ticks = {w.name: None for w in workers}
    first_dispatch_sec = {w.name: None for w in workers}
    finish_sec = {w.name: None for w in workers}
    last_kind = {}
    clock = 0

    def remaining(name):
        return by_name[name].burst - done[name]

    def missed_arrival(name, interval_end):
        """Reports an arrival the policy owed a preemption at.

        The interval runs from the worker's dispatch, exclusive, to the event
        that ends the slice, exclusive. The dispatch tick is out because a job
        that arrived then was already in the queue. `interval_end` is out
        because an arrival at a preempt tick is the arrival that caused it, and
        an arrival after a preempt sits inside the stop's overrun, which the
        controller cannot act on. The remaining work is read from the dispatch's
        done count, so it is the work the worker held at the arrival, whatever
        the later events recorded.
        """
        for other in workers:
            if not (dispatch_clock[name] < other.arrival < interval_end):
                continue
            left = by_name[name].burst - (dispatch_done[name]
                                          + (other.arrival - dispatch_clock[name]))
            if other.burst < left and other.arrival not in preempt_ticks[name]:
                problems.append(
                    f"worker {other.name} arrived at tick {other.arrival} "
                    f"with {other.burst} ticks left, less than the "
                    f"{left} left to running worker {name}, but {name} "
                    "was not preempted there")

    for (event_clock, _, _, position, name, event) in events:
        for worker in workers:
            if not released[worker.name] and worker.arrival <= event_clock:
                released[worker.name] = True
                queue.append(worker.name)

        kind = event["kind"]

        if kind == "dispatch":
            if running is not None:
                problems.append(
                    f"worker {name} is dispatched at tick {event_clock} while "
                    f"worker {running} is still running; one CPU runs one worker")
            if event["done"] != done[name]:
                problems.append(
                    f"worker {name}: the dispatch at tick {event_clock} says "
                    f"{event['done']} done, but the worker last held {done[name]}; "
                    "a stopped worker cannot run")
            if policy == "stcf":
                ready = [w.name for w in workers
                         if released[w.name] and not finished[w.name]]
                expected = min(ready, key=lambda n: (remaining(n),
                                                     by_name[n].arrival,
                                                     order_index[n])) if ready else None
            else:
                expected = queue[0] if queue else None
            if expected != name:
                problems.append(
                    f"the dispatch at tick {event_clock} runs {name}, but the "
                    f"policy picks {expected}")
            if name in queue:
                queue.remove(name)
            running = name
            dispatches[name] += 1
            dispatch_clock[name] = event_clock
            dispatch_done[name] = event["done"]
            preempt_ticks[name] = set()
            if response_ticks[name] is None:
                response_ticks[name] = event_clock - by_name[name].arrival
                first_dispatch_sec[name] = event["seconds"]

        elif kind == "preempt":
            if running != name:
                problems.append(
                    f"worker {name} is preempted at tick {event_clock} but it is "
                    f"not the running worker ({running})")
            if name in dispatch_clock:
                expected = dispatch_done[name] + (event_clock - dispatch_clock[name])
                if event["done"] != expected:
                    problems.append(
                        f"worker {name}: the preempt at tick {event_clock} says "
                        f"{event['done']} done, but the slice from its dispatch at "
                        f"tick {dispatch_clock[name]} gives {expected}")
            else:
                problems.append(f"worker {name} is preempted without a dispatch")
            if policy == "rr":
                length = event_clock - dispatch_clock.get(name, event_clock)
                if length != quantum:
                    problems.append(
                        f"worker {name}: a slice of {length} ticks ends in a "
                        f"preemption, but the quantum is {quantum}")
            elif policy == "stcf":
                # The arriving job must be strictly shorter than the running
                # worker's remaining work. Remaining work is the burst minus
                # the preempt event's done count. A tie keeps the running worker.
                left = by_name[name].burst - event["done"]
                shorter = any(
                    w.name != name and w.arrival == event_clock
                    and w.burst < left
                    for w in workers)
                if not shorter:
                    problems.append(
                        f"worker {name} is preempted at tick {event_clock} under "
                        "stcf, but no shorter job arrives there")
                if name in dispatch_clock:
                    missed_arrival(name, event_clock)
            elif policy == "fifo":
                problems.append(
                    f"worker {name} is preempted at tick {event_clock} under "
                    "fifo, but fifo never preempts")
            done[name] = event["done"]
            preempt_clock[name] = event_clock
            preempt_done[name] = event["done"]
            preempt_ticks[name].add(event_clock)
            preemptions[name] += 1

        elif kind == "stopped":
            if running != name:
                problems.append(
                    f"worker {name} stops at tick {event_clock} but it is not the "
                    f"running worker ({running})")
            base_done = preempt_done.get(name, done[name])
            base_clock = preempt_clock.get(name, event_clock)
            expected = base_done + (event_clock - base_clock)
            if event["done"] != expected:
                problems.append(
                    f"worker {name}: the stopped event at tick {event_clock} says "
                    f"{event['done']} done, but the overrun from the preempt at "
                    f"tick {base_clock} gives {expected}")
            done[name] = event["done"]
            last_stopped[name] = event["done"]
            if policy != "stcf":
                queue.append(name)
            running = None

        elif kind == "complete":
            if running != name:
                problems.append(
                    f"worker {name} completes at tick {event_clock} but it is not "
                    f"the running worker ({running})")
            if event["done"] != by_name[name].burst:
                problems.append(
                    f"worker {name}: completes with {event['done']} done, but its "
                    f"burst is {by_name[name].burst}")
            if name not in dispatch_clock:
                problems.append(f"worker {name} completes without a dispatch")
            if name in dispatch_clock:
                expected = dispatch_done[name] + (event_clock - dispatch_clock[name])
                if event["done"] != expected:
                    problems.append(
                        f"worker {name}: the complete at tick {event_clock} says "
                        f"{event['done']} done, but its dispatch at tick "
                        f"{dispatch_clock[name]} gives {expected}")
                length = event_clock - dispatch_clock[name]
                if policy == "rr" and length > quantum and last_kind.get(name) != "preempt":
                    problems.append(
                        f"worker {name}: a completed slice of {length} ticks is "
                        f"longer than the quantum {quantum}, and no preemption at "
                        "the quantum precedes it; a running worker advanced past "
                        "its slice")
                if policy == "stcf" and last_kind.get(name) != "preempt":
                    # A preempt already scanned the interval from the dispatch
                    # to the preempt tick. When no preempt came first, this
                    # completion is the event that ends the decision interval.
                    # An arrival inside a preempt's overrun is not a decision.
                    missed_arrival(name, event_clock)
            done[name] = event["done"]
            finished[name] = True
            turnaround_ticks[name] = event_clock - by_name[name].arrival
            finish_sec[name] = event["seconds"]
            running = None

        last_kind[name] = kind
        clock = max(clock, event_clock)

        # The work clock counts finished ticks, so at every event the workers'
        # done counts must add up to it. A larger sum means two workers ran at
        # once. A smaller one means a tick advanced the clock without a worker.
        total_done = sum(done.values())
        if total_done != event_clock:
            problems.append(
                f"at tick {event_clock} the workers have finished {total_done} "
                "ticks in total, so the work clock and the work disagree")

    for worker in workers:
        if not finished[worker.name]:
            problems.append(f"worker {worker.name} never completes its burst")

    # Every finished worker did its whole burst, so the clock at the end is the
    # total burst of the workers that finished. A clock that is short or long
    # means work is missing or was invented.
    finished_burst = sum(w.burst for w in workers if finished[w.name])
    if clock != finished_burst:
        problems.append(
            f"the run ends at clock {clock}, but the finished workers' bursts "
            f"total {finished_burst}")

    return {
        "dispatches": dispatches,
        "preemptions": preemptions,
        "response_ticks": response_ticks,
        "turnaround_ticks": turnaround_ticks,
        "first_dispatch_sec": first_dispatch_sec,
        "finish_sec": finish_sec,
        "clock": clock,
        "run_sequence": [(name, event["kind"]) for (_, _, _, _, name, event) in events],
    }


# --------------------------------------------------------------------------
# Running a case
# --------------------------------------------------------------------------


def wait_until_running(trace_dir, process):
    """
    Waits until the controller has written its first event, so the run is live.

    A fixed delay cannot serve here. A faster compiler turns the same workload
    into a shorter run, and an interrupt that arrives after the run has ended
    tests nothing. The first event appears within a tick or two, so this wait
    is prompt on any machine.
    """
    deadline = time.time() + TRACE_WAIT_SECONDS
    while time.time() < deadline:
        if process.poll() is not None:
            return False
        for path in trace_dir.glob("*.trace"):
            if any(line and not line.startswith("#")
                   for line in path.read_text(encoding="utf-8").splitlines()):
                return True
        time.sleep(0.005)
    return False


def run_program(cmd, repo_root, trace_dir=None, interrupt=None):
    """
    Runs the entry point and returns (stdout, stderr, code, survivors).

    The runner waits for the controller, not for pipe EOF. A worker can outlive
    the controller and keep an inherited pipe open. A pipe read would then block
    and make the run look slow. The runner records survivors before cleanup.
    A timeout marks a run that hangs.
    """
    process = subprocess.Popen(
        cmd,
        cwd=repo_root,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8",
        errors="replace",
        start_new_session=True,
    )

    if interrupt:
        time.sleep(INTERRUPT_DELAY_MS / 1000.0)
        if process.poll() is None and trace_dir is not None:
            wait_until_running(trace_dir, process)
        if process.poll() is None:
            time.sleep(INTERRUPT_SETTLE_MS / 1000.0)
        if process.poll() is None:
            process.send_signal(signal.SIGINT)

    timed_out = False
    try:
        process.wait(timeout=TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        timed_out = True
        kill_tree(process)
        try:
            process.wait(timeout=10)
        except subprocess.TimeoutExpired:
            pass

    survivors = running_processes()
    if survivors:
        kill_leftovers()

    try:
        stdout, stderr = process.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        stdout, stderr = "", ""

    if timed_out:
        return stdout, stderr + (f"\nTIMEOUT: the run took more than "
                                 f"{TIMEOUT_SECONDS}s.\n"), -1, survivors
    return stdout, stderr, process.returncode, survivors


def kill_tree(process):
    try:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
    except OSError:
        pass
    process.kill()


def running_processes():
    """Returns matching comm values, or None if ps fails."""
    try:
        result = subprocess.run(["ps", "-A", "-o", "comm="], capture_output=True,
                                text=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        return None
    if result.returncode != 0:
        return None
    return [line.strip() for line in result.stdout.splitlines()
            if PROCESS_NAME in line]


def kill_leftovers():
    """Removes any process the program left behind, so later cases start clean.

    The pattern keeps the bracket trick so it cannot match the runner itself.
    """
    try:
        subprocess.run(["pkill", "-9", "-f", f"[{PROCESS_NAME[0]}]{PROCESS_NAME[1:]}"],
                       capture_output=True, timeout=10)
    except (OSError, subprocess.SubprocessError):
        pass


# --------------------------------------------------------------------------
# Comparisons
# --------------------------------------------------------------------------


def parse_report(stdout, problems):
    """Parses the report. Returns None when the shape is wrong."""
    lines = [line for line in stdout.splitlines() if line.strip()]
    if len(lines) < 3:
        problems.append(f"the report holds {len(lines)} lines, which is too few")
        return None

    header = HEADER_RE.match(lines[0])
    if not header:
        problems.append(f"the header line is '{lines[0]}'")
        return None

    result = RESULT_RE.match(lines[-1])
    if not result:
        problems.append(f"the last line is '{lines[-1]}', not a result line")
        return None

    average = AVERAGE_RE.match(lines[-2])
    if not average:
        problems.append(f"the average line is '{lines[-2]}'")
        return None

    workers = []
    for line in lines[1:-2]:
        match = WORKER_RE.match(line)
        if not match:
            problems.append(f"the worker line is '{line}'")
            return None
        workers.append({
            "name": match.group(1),
            "arrival": int(match.group(2)),
            "burst": int(match.group(3)),
            "response_ticks": match.group(4),
            "turnaround_ticks": match.group(5),
            "response_ms": match.group(6),
            "turnaround_ms": match.group(7),
            "dispatches": int(match.group(8)),
            "preemptions": int(match.group(9)),
        })

    return {
        "policy": header.group(1),
        "quantum": int(header.group(2)),
        "workers": int(header.group(3)),
        "ticks": int(header.group(4)),
        "result": result.group(1),
        "average": {"response_ticks": average.group(1),
                    "turnaround_ticks": average.group(2),
                    "response_ms": average.group(3),
                    "turnaround_ms": average.group(4)},
        "rows": workers,
    }


def mean(values):
    present = [v for v in values if v is not None]
    if not present:
        return None
    return sum(present) / len(present)


def check_report(report, workers, metrics, policy, quantum, problems):
    """Compares the report's tick columns against the replay, exactly."""
    if report["policy"] != policy:
        problems.append(f"the report says policy {report['policy']}, expected {policy}")
    if report["quantum"] != quantum:
        problems.append(f"the report says quantum {report['quantum']}, "
                        f"expected {quantum}")
    if report["workers"] != len(workers):
        problems.append(f"the report says {report['workers']} workers, "
                        f"expected {len(workers)}")
    if report["result"] != "finished":
        problems.append(f"the result line says '{report['result']}', expected "
                        "'finished'")
    if report["ticks"] != metrics["clock"]:
        problems.append(f"the header says {report['ticks']} ticks, but the replay "
                        f"ends at {metrics['clock']}")
    if len(report["rows"]) != len(workers):
        problems.append(f"the report holds {len(report['rows'])} worker lines, "
                        f"expected {len(workers)}")
        return

    for row, worker in zip(report["rows"], workers):
        if row["name"] != worker.name:
            problems.append(f"a worker line names '{row['name']}', expected "
                            f"'{worker.name}'")
            continue
        if row["arrival"] != worker.arrival or row["burst"] != worker.burst:
            problems.append(f"worker {worker.name}: arrival {row['arrival']} burst "
                            f"{row['burst']}, expected arrival {worker.arrival} "
                            f"burst {worker.burst}")
        if row["dispatches"] != metrics["dispatches"][worker.name]:
            problems.append(f"worker {worker.name}: {row['dispatches']} dispatches, "
                            f"expected {metrics['dispatches'][worker.name]}")
        if row["preemptions"] != metrics["preemptions"][worker.name]:
            problems.append(f"worker {worker.name}: {row['preemptions']} preemptions, "
                            f"expected {metrics['preemptions'][worker.name]}")
        want = metrics["response_ticks"][worker.name]
        if row["response_ticks"] == "-":
            problems.append(f"worker {worker.name}: response_ticks is '-', "
                            f"expected {want}")
        elif want is None or int(row["response_ticks"]) != want:
            problems.append(f"worker {worker.name}: response_ticks is "
                            f"{row['response_ticks']}, expected {want}")
        want = metrics["turnaround_ticks"][worker.name]
        if row["turnaround_ticks"] == "-":
            problems.append(f"worker {worker.name}: turnaround_ticks is '-', "
                            f"expected {want}")
        elif want is None or int(row["turnaround_ticks"]) != want:
            problems.append(f"worker {worker.name}: turnaround_ticks is "
                            f"{row['turnaround_ticks']}, expected {want}")

    for field, key in (("response_ticks", "response_ticks"),
                       ("turnaround_ticks", "turnaround_ticks")):
        values = [metrics[key][w.name] for w in workers]
        want = mean([float(v) for v in values if v is not None])
        text = report["average"][field]
        if want is None:
            continue
        if text == "-":
            problems.append(f"the average {field} is '-', expected {want:.2f}")
        elif abs(float(text) - want) > 0.01:
            problems.append(f"the average {field} is {text}, expected {want:.2f}")


def check_measured(report, workers, metrics, traces, problems):
    """Recomputes every measured time from the trace and checks the report.

    This path checks a completed run. Every worker has a dispatch and finish
    time. Each report row must include both times. The report averages must
    match the worker rows. The replay records each worker's first dispatch and
    finish times. A report uses `-` only when the run leaves a worker undispatched
    or unfinished. The partial-run checker handles those rows.
    """
    for row in report["rows"]:
        name = row["name"]
        if name not in traces:
            continue
        arrival_time = traces[name].arrival_time
        record = metrics
        if row["response_ms"] == "-":
            if record["first_dispatch_sec"][name] is not None:
                problems.append(f"worker {name}: response_ms is '-' but the "
                                "worker ran in a finished run")
        else:
            want = record["first_dispatch_sec"][name]
            if want is None:
                problems.append(f"worker {name}: response_ms is {row['response_ms']} "
                                "but the trace holds no dispatch")
            elif abs(float(row["response_ms"]) - (want - arrival_time) * 1000.0) > MS_TOLERANCE:
                got = (want - arrival_time) * 1000.0
                problems.append(f"worker {name}: response_ms is {row['response_ms']}, "
                                f"but the trace gives {got:.6f}")
        if row["turnaround_ms"] == "-":
            if record["finish_sec"][name] is not None:
                problems.append(f"worker {name}: turnaround_ms is '-' but the "
                                "worker finished in a finished run")
        else:
            want = record["finish_sec"][name]
            if want is None:
                problems.append(f"worker {name}: turnaround_ms is "
                                f"{row['turnaround_ms']} but the trace holds no "
                                "completion")
            elif abs(float(row["turnaround_ms"]) - (want - arrival_time) * 1000.0) > MS_TOLERANCE:
                got = (want - arrival_time) * 1000.0
                problems.append(f"worker {name}: turnaround_ms is "
                                f"{row['turnaround_ms']}, but the trace gives "
                                f"{got:.6f}")

    for field, source in (("response_ms", "response_ms"),
                          ("turnaround_ms", "turnaround_ms")):
        values = []
        for row in report["rows"]:
            if row[source] != "-":
                values.append(float(row[source]))
        want = mean(values)
        text = report["average"][field]
        if want is None:
            continue
        if text == "-":
            problems.append(f"the average {field} is '-', expected {want:.6f}")
        elif abs(float(text) - want) > MS_TOLERANCE:
            problems.append(f"the average {field} is {text}, expected {want:.6f} "
                            "from the rows")


def check_rate(label, per_tick, problems):
    """Checks one per-tick rate against the allowed window.

    A rate below the floor indicates a failed time measurement. The checker
    reports that failure. A rate above the ceiling may reflect host load. The
    checker identifies the affected slice and lets the case pass. A high rate
    signals a rerun.
    """
    if per_tick < MS_PER_TICK_MIN:
        problems.append(
            f"{label} costs {per_tick:.3f} ms per tick, below the "
            f"{MS_PER_TICK_MIN} ms floor; the controller is not measuring, or "
            "it reads the dispatch time after SIGCONT, or the tick's work was "
            "optimized away")
    elif per_tick > MS_PER_TICK_MAX:
        print(f"       [info] {label} costs {per_tick:.3f} ms per tick, over the "
              f"{MS_PER_TICK_MAX} ms ceiling; the host was busy, so run the case "
              "again")


def check_slice_rate(metrics, traces, problems):
    """Checks every measured rate against the window.

    The window applies to each slice, to each worker's turnaround, and to the
    run's average, so each one is measured per tick and handed to check_rate.
    """
    for name, trace in traces.items():
        events = trace.events
        for position in range(len(events)):
            if events[position]["kind"] != "dispatch":
                continue
            start = events[position]
            if position + 1 >= len(events):
                continue
            end = events[position + 1]
            span = end["clock"] - start["clock"]
            if span <= 0:
                continue
            per_tick = (end["seconds"] - start["seconds"]) * 1000.0 / span
            check_rate(f"worker {name}: a slice of {span} ticks", per_tick, problems)

    turnaround_ticks = metrics.get("turnaround_ticks", {})
    finish_sec = metrics.get("finish_sec", {})
    total_ticks = 0
    total_ms = 0.0
    for name, trace in traces.items():
        ticks = turnaround_ticks.get(name)
        finish = finish_sec.get(name)
        if ticks is None or finish is None or ticks <= 0:
            continue
        gap_ms = (finish - trace.arrival_time) * 1000.0
        check_rate(f"worker {name}: a turnaround of {ticks} ticks",
                   gap_ms / ticks, problems)
        total_ticks += ticks
        total_ms += gap_ms
    if total_ticks > 0:
        check_rate(f"the run: an average turnaround over {total_ticks} ticks",
                   total_ms / total_ticks, problems)


def check_partial(report, workers, traces, problems):
    """Checks a partial report against its traces, without a schedule.

    Each row carries a dispatch count and a preemption count. Both must match
    the events in that worker's trace. A worker whose stop lands after its burst
    is done finishes inside the overrun. That worker can hold one preemption and
    one dispatch, so the row is compared with its trace and not with a rule.
    """
    by_name = {w.name: w for w in workers}
    if len(report["rows"]) != len(workers):
        problems.append(f"the partial report holds {len(report['rows'])} worker "
                        f"lines, expected {len(workers)}")
        return
    if report["result"] == "finished":
        problems.append("the result line says 'finished', expected 'interrupted' "
                        "or 'faulted'")
    if report["ticks"] > sum(w.burst for w in workers):
        problems.append(f"the partial report ran {report['ticks']} ticks, more than "
                        f"the workload's {sum(w.burst for w in workers)}")
    for row in report["rows"]:
        worker = by_name.get(row["name"])
        if worker is None:
            problems.append(f"the partial report names an unknown worker "
                            f"'{row['name']}'")
            continue
        if row["arrival"] != worker.arrival or row["burst"] != worker.burst:
            problems.append(f"worker {row['name']}: arrival {row['arrival']} burst "
                            f"{row['burst']}, expected arrival {worker.arrival} "
                            f"burst {worker.burst}")
        if row["response_ticks"] == "-" and row["dispatches"] != 0:
            problems.append(f"worker {row['name']}: response_ticks is '-' but the "
                            f"worker holds {row['dispatches']} dispatches")
        trace = traces.get(row["name"])
        if trace is not None:
            dispatches = sum(1 for event in trace.events
                             if event["kind"] == "dispatch")
            preemptions = sum(1 for event in trace.events
                              if event["kind"] == "preempt")
            if row["dispatches"] != dispatches:
                problems.append(f"worker {row['name']}: the report says "
                                f"{row['dispatches']} dispatches, but its trace "
                                f"holds {dispatches}")
            if row["preemptions"] != preemptions:
                problems.append(f"worker {row['name']}: the report says "
                                f"{row['preemptions']} preemptions, but its trace "
                                f"holds {preemptions}")
        if row["turnaround_ticks"] != "-":
            if row["response_ticks"] != "-" and \
                    int(row["turnaround_ticks"]) < int(row["response_ticks"]):
                problems.append(f"worker {row['name']}: turnaround "
                                f"{row['turnaround_ticks']} is below response "
                                f"{row['response_ticks']}")


def check_trace_shape(trace_dir, workers, problems):
    """Checks each partial trace for a legal event order, without a schedule."""
    for worker in workers:
        path = trace_dir / f"{worker.name}.trace"
        if not path.exists():
            problems.append(f"the trace file '{worker.name}.trace' is missing")
            continue
        try:
            trace = parse_trace(path)
        except CaseError as error:
            problems.append(str(error))
            continue
        events = trace.events
        position = 0
        while position < len(events):
            if events[position]["kind"] != "dispatch":
                problems.append(f"trace {worker.name}: event {position + 1} is "
                                f"'{events[position]['kind']}', expected 'dispatch'")
                break
            position += 1
            if position == len(events):
                break
            kind = events[position]["kind"]
            if kind == "complete":
                position += 1
                if position != len(events):
                    problems.append(f"trace {worker.name}: an event follows the "
                                    "'complete'")
                break
            if kind != "preempt":
                problems.append(f"trace {worker.name}: event {position + 1} is "
                                f"'{kind}', expected 'preempt' or 'complete'")
                break
            position += 1
            if position == len(events):
                break
            if events[position]["kind"] == "complete":
                # The worker finished in the moment the stop took effect.
                position += 1
                if position != len(events):
                    problems.append(f"trace {worker.name}: an event follows the "
                                    "'complete'")
                break
            if events[position]["kind"] != "stopped":
                problems.append(f"trace {worker.name}: event {position + 1} is "
                                f"'{events[position]['kind']}', expected 'stopped'")
                break
            position += 1

        previous = 0
        for event in events:
            if event["done"] < previous:
                problems.append(f"trace {worker.name}: the done count falls from "
                                f"{previous} to {event['done']}")
            previous = event["done"]
            if event["seconds"] <= 0.0:
                problems.append(f"trace {worker.name}: a timestamp is not positive")
        if events and events[-1]["done"] > worker.burst:
            problems.append(f"trace {worker.name}: {events[-1]['done']} done "
                            f"exceeds the burst of {worker.burst}")


# --------------------------------------------------------------------------
# The case loop
# --------------------------------------------------------------------------


def run_case(work_path, repo_root):
    """Runs one case and returns a list of problems, empty when it passes."""
    problems = []
    name = work_path.relative_to(repo_root)

    spec = parse_expect(work_path.with_suffix(".expect"))
    args = parse_args_file(work_path.with_suffix(".args"))
    workers = [w for w in parse_work(work_path) if w is not None]
    policy = policy_of(args)
    quantum = quantum_of(args)

    trace_dir = Path(tempfile.mkdtemp(prefix="ps1-trace-"))
    try:
        try:
            target = work_path.relative_to(repo_root).as_posix()
        except ValueError:
            target = str(work_path)
        if spec["path"] is not None:
            target = spec["path"]
        cmd = ["./run"] + args + ["--trace-dir", str(trace_dir), target]
        stdout, stderr, code, survivors = run_program(cmd, repo_root, trace_dir,
                                                      spec["interrupt"])

        if code != spec["exit"]:
            problems.append(f"exit code: expected {spec['exit']}, got {code}")
        for wanted in spec["stderr"]:
            if wanted not in stderr:
                problems.append(f"stderr: expected to find '{wanted}', got "
                                f"'{stderr.strip()}'")

        if spec["schedule"] is not None and workers and not problems:
            expect_dispatches = expect_from_schedule(spec["schedule"])
            model_dispatches, _ = policy_engine(workers, policy, quantum)
            if model_dispatches != expect_dispatches:
                problems.append(
                    f"[case] the .expect schedule does not follow the policy: the "
                    f"model gives {model_dispatches}, the case states "
                    f"{expect_dispatches}")

        if code == 0 and not problems:
            if spec["schedule"] is not None:
                paper = [f"{w}@{t}" for w, t in expect_from_schedule(spec["schedule"])]
                print(f"       [info] paper schedule for zero overrun: "
                      f"{' '.join(paper)}")
            traces = load_traces(trace_dir, workers, problems)
            if traces:
                metrics = replay(workers, policy, quantum, traces, problems)
                report = parse_report(stdout, problems)
                if metrics and report:
                    check_report(report, workers, metrics, policy, quantum, problems)
                    check_measured(report, workers, metrics, traces, problems)
                check_slice_rate(metrics or {}, traces, problems)
                if spec["schedule"] is not None:
                    print("       [info] the run's decisions: "
                          + " ".join(f"{n}:{k}" for n, k in metrics["run_sequence"]))
        elif spec["partial"]:
            report = parse_report(stdout, problems)
            if report:
                check_partial(report, workers,
                              load_partial_traces(trace_dir, workers), problems)
            check_trace_shape(trace_dir, workers, problems)

        if survivors is None:
            problems.append("the runner could not list processes, so it could not "
                            "check the cleanup contract")
        elif survivors:
            problems.append(f"{len(survivors)} process(es) survived the run: "
                            + ", ".join(sorted(set(survivors))[:4]))

        return name, problems
    finally:
        shutil.rmtree(trace_dir, ignore_errors=True)


def main():
    parser = argparse.ArgumentParser(description="CMSC 125 Problem Set 1 case runner.")
    parser.add_argument("case_folder", help="Folder that holds the .work cases, such as cases")
    parser.add_argument("--repo-root", default=".",
                        help="Repository root. Defaults to the current directory.")
    args = parser.parse_args()

    repo_root = Path(args.repo_root).resolve()
    case_folder = Path(args.case_folder).resolve()

    if not case_folder.exists():
        print(f"ERROR: case folder '{case_folder}' does not exist.", file=sys.stderr)
        return 1

    entry = repo_root / "run"
    if not entry.exists():
        print("ERROR: './run' is missing. Is this the starter's root directory?",
              file=sys.stderr)
        return 1
    if not os.access(entry, os.X_OK):
        print("ERROR: './run' is not executable. Run 'chmod +x run check.sh'.",
              file=sys.stderr)
        return 1

    work_files = sorted(case_folder.rglob("*.work"))
    if not work_files:
        print(f"ERROR: no .work files under '{case_folder}'.", file=sys.stderr)
        return 1

    before = running_processes()
    if before:
        print("WARNING: a cmsc-125-ps1 process is already running. The cleanup "
              "check counts only what each case leaves behind.", file=sys.stderr)

    failures = []
    for work_path in work_files:
        try:
            name, problems = run_case(work_path, repo_root)
        except CaseError as error:
            name, problems = work_path.relative_to(repo_root), [str(error)]
        if problems:
            failures.append(name)
            print(f"[FAIL] {name}")
            for line in "\n".join(problems).splitlines():
                print(f"       {line}")
        else:
            print(f"[PASS] {name}")

    total = len(work_files)
    print(f"\n{total - len(failures)}/{total} cases passed.")
    return 0 if not failures else 1


if __name__ == "__main__":
    sys.exit(main())
