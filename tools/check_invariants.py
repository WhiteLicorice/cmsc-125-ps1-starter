#!/usr/bin/env python3
"""
check_invariants.py: checks the invariants outside the published case corpus.

These checks are part of the check. `check.sh` runs them after the public
corpus. Each one exercises a claim that the case corpus cannot state on its
own:

1. A worker killed by a signal is a fault, and nothing survives it.
2. A fork that fails after one child exists is a fault, and nothing survives
   it. The check loads a small shim through LD_PRELOAD. The shim prints a load
   marker when the loader loads it, and a refusal marker when a `fork` call
   reaches it. A run with no load marker, or a run that exits 0 with no refusal,
   cannot prove the fork behaviour, so it reports a skip. A run that exits
   nonzero with no refusal reports a failure that names its exit code and its
   first line of standard error. A shim that fails to compile reports a failure
   with the compiler diagnostic.
3. The controller signals only its own children. An unrelated process is
   still alive after a run.
4. Each run of one workload follows the policy. Two runs do not have to make
   the same decisions, because the overrun can use up a worker's remaining
   burst or move a later decision, so the check replays each run from its own
   traces and confirms the decisions that run made. The replay is the
   published runner's, so the check and the corpus agree.

Usage: python3 check_invariants.py <work directory with a built starter>
Exit code: 0 when every check passes, 1 otherwise.
"""

import importlib.util
import os
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

TIMEOUT = 30
PROCESS_NAME = "cmsc-125-ps1"


class Skip:
    """A check that could not run. The reason prints under the [SKIP] line."""

    def __init__(self, reason):
        self.reason = reason


def run(command, cwd, timeout=TIMEOUT):
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True,
                            timeout=timeout)
    return result.stdout, result.stderr, result.returncode


def processes_named(name):
    result = subprocess.run(["ps", "-A", "-o", "pid,comm"], capture_output=True,
                            text=True, timeout=10)
    found = []
    for line in result.stdout.splitlines()[1:]:
        fields = line.split(None, 1)
        if len(fields) == 2 and name in fields[1]:
            found.append(int(fields[0]))
    return found


def children_of(pid):
    result = subprocess.run(["ps", "-A", "-o", "pid,ppid"], capture_output=True,
                            text=True, timeout=10)
    found = []
    for line in result.stdout.splitlines()[1:]:
        fields = line.split()
        if len(fields) == 2 and fields[1].isdigit() and int(fields[1]) == pid:
            found.append(int(fields[0]))
    return found


def kill_leftovers():
    subprocess.run(["pkill", "-9", "-f", f"[{PROCESS_NAME[0]}]{PROCESS_NAME[1:]}"],
                   capture_output=True)


def trace_has_event(trace_dir):
    """True once any trace file holds an event, so the run is under way."""
    for path in trace_dir.glob("*.trace"):
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.lstrip().startswith("#"):
                return True
    return False


def check_killed_worker(work):
    """A worker killed by SIGKILL is reported as a fault, and nothing survives."""
    traces = work / "traces-killed"
    process = subprocess.Popen(
        ["./run", "--policy", "fifo", "--trace-dir", str(traces),
         "cases/cleanup/sigint_mid_run.work"],
        cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
        start_new_session=True)

    # A fixed delay cannot serve here. A faster compiler turns the same workload
    # into a shorter run. A check that kills nothing tests nothing. Wait for
    # the first event, then give the run a moment to keep going. The check waits
    # for the first trace line. The controller creates the trace file before any
    # event.
    deadline = time.time() + 5.0
    while time.time() < deadline and process.poll() is None:
        if trace_has_event(traces):
            break
        time.sleep(0.005)
    time.sleep(0.02)

    killed = 0
    for pid in children_of(process.pid):
        try:
            os.kill(pid, signal.SIGKILL)
            killed += 1
        except ProcessLookupError:
            pass
    if killed == 0:
        process.kill()
        return "the runner found no worker to kill"

    try:
        stdout, stderr = process.communicate(timeout=TIMEOUT)
    except subprocess.TimeoutExpired:
        os.killpg(os.getpgid(process.pid), signal.SIGKILL)
        stdout, stderr = process.communicate(timeout=10)
        return "the controller did not exit after a worker was killed"

    problems = []
    if process.returncode != 70:
        problems.append(f"exit code: expected 70, got {process.returncode}")
    if "signal 9" not in stderr:
        problems.append(f"stderr does not name the signal: '{stderr.strip()}'")
    if "result faulted" not in stdout:
        problems.append("the report does not end in 'result faulted'")
    time.sleep(0.05)
    survivors = processes_named(PROCESS_NAME)
    if survivors:
        problems.append(f"{len(survivors)} process(es) survived the fault")
        kill_leftovers()
    return "; ".join(problems)


def check_unrelated_process(work):
    """A process the controller did not fork is still alive after a run."""
    bystander = subprocess.Popen(["sleep", "4"], start_new_session=True)
    try:
        time.sleep(0.1)
        if bystander.poll() is not None:
            return "the bystander exited before the run"
        stdout, stderr, code = run(
            ["./run", "--policy", "stcf", "--trace-dir", str(work / "traces-bystander"),
             "cases/normal/stcf_simultaneous.work"], work)
        if code != 0:
            return f"the run failed: exit {code}, stderr '{stderr.strip()}'"
        if bystander.poll() is not None:
            return "the controller signalled a process it did not fork"
        return ""
    finally:
        bystander.kill()
        bystander.wait(timeout=10)


def load_checker(work):
    """Loads the published case runner from the work copy."""
    path = work / "tools" / "check_cases.py"
    spec = importlib.util.spec_from_file_location("check_cases", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check_policy_validity(work):
    """Each run of one workload follows the policy.

    Two runs of one workload do not have to make the same decisions. The
    overrun can use up a worker's remaining burst, move a later decision, or
    shift a round-robin slice end, so comparing two runs with each other proves
    nothing about a third. This check replays each run from its own traces with
    the published runner and confirms the decisions that run's policy made.
    """
    workload = work / "reproducible.work"
    workload.write_text("A 0 100\nB 0 150\n", encoding="utf-8")
    checker = load_checker(work)
    workers = [w for w in checker.parse_work(workload) if w is not None]
    policy = "rr"
    quantum = 50

    for index, target in enumerate((work / "traces-repro-1",
                                    work / "traces-repro-2"), 1):
        stdout, stderr, code = run(
            ["./run", "--policy", policy, "--quantum", str(quantum),
             "--trace-dir", str(target), "reproducible.work"], work)
        if code != 0:
            return f"run {index} failed: exit {code}, stderr '{stderr.strip()}'"
        problems = []
        traces = checker.load_traces(target, workers, problems)
        if traces:
            checker.replay(workers, policy, quantum, traces, problems)
        if problems:
            return f"run {index} does not follow the policy: " + "; ".join(problems)
    return ""


# The shim makes the second `fork` fail with EAGAIN, after one child already
# exists. It prints two markers. The load marker comes from a constructor, so
# it appears whenever the loader loads the library in any process of the run.
# The refusal marker appears only when a `fork` call reaches the shim. A
# missing load marker means no process loaded the library. A missing refusal
# marker means no `fork` call reached it. The exit code tells the two cases of
# a missing refusal apart: exit 0 is a finished run whose forks never reached
# the shim, and a nonzero exit is a run that failed for its own reason.
FORK_SHIM_SOURCE = """
#define _GNU_SOURCE
#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>

__attribute__((constructor))
static void ps1_fork_shim_loaded(void)
{
    fprintf(stderr, "ps1-fork-shim: loaded\\n");
}

pid_t fork(void)
{
    static int calls;
    pid_t (*real_fork)(void) = (pid_t (*)(void))dlsym(RTLD_NEXT, "fork");

    if (++calls == 2) {
        fprintf(stderr, "ps1-fork-shim: refusing fork\\n");
        errno = EAGAIN;
        return -1;
    }
    return real_fork();
}
"""

FORK_SHIM_MARKER = "ps1-fork-shim: refusing fork"
FORK_SHIM_LOAD_MARKER = "ps1-fork-shim: loaded"


def build_fork_shim(directory):
    """Compiles the shim. Returns its path, or raises when it cannot build."""
    source = directory / "fork_refusal.c"
    source.write_text(FORK_SHIM_SOURCE, encoding="utf-8")
    suffix = "dylib" if sys.platform == "darwin" else "so"
    library = directory / f"fork_refusal.{suffix}"
    command = [os.environ.get("CC", "cc"), "-shared", "-fPIC", "-o",
               str(library), str(source)]
    if sys.platform != "darwin":
        command.append("-ldl")
    result = subprocess.run(command, capture_output=True, text=True, timeout=60)
    if result.returncode != 0:
        diagnostic = result.stderr.strip() or result.stdout.strip()
        if diagnostic:
            raise RuntimeError(diagnostic)
        raise RuntimeError(f"the compiler exited with status {result.returncode}")
    return library


def preload_environment(library):
    """Both preload variables are set. Each platform reads the one it knows."""
    environment = os.environ.copy()
    environment["LD_PRELOAD"] = str(library)
    environment["DYLD_INSERT_LIBRARIES"] = str(library)
    return environment


def check_fork_failure(work):
    """A failed fork after one child starts is a fault. The check must find no
    worker process left alive.

    Returns a `Skip` when the run cannot prove the fork behaviour. That happens
    when no process loaded the shim, or when the run finished with no `fork`
    call reaching it, because a static controller and a loader that skips the
    binary both hide the interposed `fork`. A run that failed for its own
    reason is a failure, not a skip.
    """
    directory = Path(tempfile.mkdtemp(prefix="ps1-fork-shim-"))
    try:
        try:
            library = build_fork_shim(directory)
        except (OSError, subprocess.SubprocessError, RuntimeError) as error:
            return f"the shim did not compile: {error}"
        process = subprocess.Popen(
            ["./run", "--policy", "fifo", "cases/normal/fifo_three.work"],
            cwd=work, env=preload_environment(library), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, start_new_session=True)
        try:
            stdout, stderr = process.communicate(timeout=TIMEOUT)
        except subprocess.TimeoutExpired:
            os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            stdout, stderr = process.communicate(timeout=10)
            return "the controller did not exit after a fork failed"

        if FORK_SHIM_MARKER in stderr:
            return fork_refused_outcome(process.returncode, stdout, stderr)

        if process.returncode != 0:
            return failed_run_outcome(process.returncode, stderr)

        # The run finished with no refusal. Either the controller's forks never
        # reached the shim, or its forks were never refused.
        if FORK_SHIM_LOAD_MARKER in stderr:
            return Skip("the shim loaded, but the controller's fork calls never "
                        "reached it, so the run proves nothing")
        return Skip("the platform did not load the shim, so no process could "
                    "interpose the controller's forks")
    finally:
        shutil.rmtree(directory, ignore_errors=True)


def first_controller_line(stderr):
    """The controller's first stderr line. The shim's own markers go first."""
    lines = [line for line in stderr.splitlines()
             if line.strip() and not line.startswith("ps1-fork-shim:")]
    return lines[0] if lines else "no output"


def failed_run_outcome(returncode, stderr):
    """A run that exited nonzero with no refused fork failed for its own
    reason. The message reports that, and claims nothing about forking."""
    return ("the run failed before any fork was refused: "
            f"exit code {returncode}, "
            f"first stderr line '{first_controller_line(stderr)}'")


def fork_refused_outcome(returncode, stdout, stderr):
    """The ordinary path: a fork was refused, so every claim applies."""
    problems = []
    if returncode != 70:
        problems.append(f"exit code: expected 70, got {returncode}")
    if "result faulted" not in stdout:
        problems.append("the report does not end in 'result faulted'")
    if "cannot fork worker" not in stderr:
        problems.append("stderr does not name the fork failure: "
                        f"'{stderr.strip()}'")
    time.sleep(0.05)
    survivors = processes_named(PROCESS_NAME)
    if survivors:
        problems.append(f"{len(survivors)} process(es) survived the fault")
        kill_leftovers()
    return "; ".join(problems)


CHECKS = [
    ("a killed worker is a fault, and it cleans up", check_killed_worker),
    ("a failed fork is a fault, and it cleans up", check_fork_failure),
    ("the controller signals only its own children", check_unrelated_process),
    ("each run follows the policy", check_policy_validity),
]


def main():
    if len(sys.argv) != 2:
        print("usage: python3 check_invariants.py <work directory>", file=sys.stderr)
        return 1
    work = Path(sys.argv[1]).resolve()
    if not (work / "run").exists():
        print(f"'{work}' holds no run entry point.", file=sys.stderr)
        return 1

    kill_leftovers()
    failures = 0
    skipped = 0
    for title, check in CHECKS:
        try:
            problem = check(work)
        except (OSError, subprocess.SubprocessError) as error:
            problem = f"the check could not run: {error}"
        if isinstance(problem, Skip):
            skipped += 1
            print(f"[SKIP] {title}\n       {problem.reason}")
        elif problem:
            failures += 1
            print(f"[FAIL] {title}\n       {problem}")
        else:
            print(f"[PASS] {title}")
    ran = len(CHECKS) - skipped
    summary = f"\n{ran - failures}/{ran} invariant checks passed"
    if skipped:
        summary += f", {skipped} skipped"
    print(summary + ".")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
