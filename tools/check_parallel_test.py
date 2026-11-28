#!/usr/bin/env python3
"""check_parallel_test.py: a second run survives the first checker.

The case checker removes what a run leaves behind. A cleanup that matches
processes by name also reaches a run in another checkout, in another
terminal, or in a group member's copy. This test starts a process that belongs to
a separate run, runs the checker over the corpus, and requires that process
to still be alive at the end. The test also runs two checkers at the same
time and requires the same result from both.

The starter's corpus is red until the student finishes it, so the two-run
part compares the runs with each other. It never requires a passing corpus.

Run it from the starter root after `make`:

    python3 tools/check_parallel_test.py
"""
import os
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from pathlib import Path

RUN_SECONDS = 300.0
ROOT = Path(__file__).resolve().parent.parent


def start_checker():
    return subprocess.Popen(
        [sys.executable, "tools/check_cases.py", "cases"],
        cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        text=True, start_new_session=True)


def finish(process):
    try:
        return process.communicate(timeout=RUN_SECONDS)
    except subprocess.TimeoutExpired:
        process.kill()
        return process.communicate()[0], "the run did not finish"


def last_line(text):
    lines = [line for line in text.splitlines() if line.strip()]
    return lines[-1] if lines else "no output"


def first_failure(text):
    """Returns the first failing case line and its detail, or the last line."""
    lines = text.splitlines()
    for index, line in enumerate(lines):
        if line.startswith("[FAIL]"):
            detail = [line.strip()]
            for extra in lines[index + 1:index + 5]:
                if extra.startswith("       ") and extra.strip():
                    detail.append(extra.strip())
                else:
                    break
            return " ".join(detail)
    return last_line(text)


def check_separate_run_survives():
    """A process from a separate run, named for the program, keeps running."""
    scratch = Path(tempfile.mkdtemp(prefix="ps1-parallel-"))
    sentinel = scratch / "cmsc-125-ps1"
    # The checker may spend 30 seconds on one hung case and more on the rest.
    # The sentinel must outlive RUN_SECONDS, or its own exit would look like a
    # kill by the checker.
    sentinel.write_text("#!/bin/sh\nsleep 600\n", encoding="utf-8")
    sentinel.chmod(0o755)
    # The sentinel gets no share of this process's output, or its sleep child
    # would hold the caller's pipe open after the check ends.
    process = subprocess.Popen([str(sentinel)], start_new_session=True,
                               stdin=subprocess.DEVNULL,
                               stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
    try:
        time.sleep(0.2)
        if process.poll() is not None:
            return "the test process exited before the checker started"
        output, error = finish(start_checker())
        if error:
            return error
        time.sleep(0.05)
        if process.poll() is not None:
            return ("the checker killed a process from a separate run "
                    f"(checker ended with '{last_line(output)}')")
        return ""
    finally:
        if process.poll() is None:
            # The sentinel is a shell with a sleep child. Kill the whole group,
            # or the sleep outlives the shell.
            try:
                os.killpg(os.getpgid(process.pid), signal.SIGKILL)
            except OSError:
                pass
            process.wait(timeout=10)
        shutil.rmtree(scratch, ignore_errors=True)


def check_two_runs_agree():
    """Two checker runs at the same time report the same result."""
    second = start_checker()
    # Give the second run a live controller before the first one starts.
    time.sleep(0.02)
    first = start_checker()

    first_output, first_error = finish(first)
    if first_error:
        return first_error
    second_output, second_error = finish(second)
    if second_error:
        return second_error

    if first.returncode != second.returncode:
        return (f"the two runs exited differently: {first.returncode} and "
                f"{second.returncode}. First run: {first_failure(first_output)}. "
                f"Second run: {first_failure(second_output)}.")
    if last_line(first_output) != last_line(second_output):
        return (f"the two runs reported differently: "
                f"'{last_line(first_output)}' and "
                f"'{last_line(second_output)}'")
    return ""


def main():
    if not (ROOT / "run").exists():
        print("the test needs ./run. Run it from the starter root.",
              file=sys.stderr)
        return 2

    problem = check_separate_run_survives()
    if problem:
        print(problem, file=sys.stderr)
        return 1
    print("[PASS] a process from a separate run survives the checker")

    problem = check_two_runs_agree()
    if problem:
        print(problem, file=sys.stderr)
        return 1
    print("[PASS] two checker runs at once report the same result")
    return 0


if __name__ == "__main__":
    sys.exit(main())
