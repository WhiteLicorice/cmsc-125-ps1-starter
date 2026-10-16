#!/usr/bin/env bash
# check.sh: run the complete public automated check.
#
# The script builds, then runs every case under the given folder. Each case
# sets its own command line, its own expected exit code, and its own expected
# schedule. The runner also checks the measured times. It confirms that no
# controller or worker process survives a case.
#
#   ./check.sh                 run the published corpus in cases/
#   ./check.sh student-cases   run the pair's own cases in student-cases/
set -u

case_folder="${1:-cases}"
failures=0

banner() {
  printf '\n== %s ==\n' "$1"
}

if command -v python3 >/dev/null 2>&1; then
  PYTHON=python3
elif command -v python >/dev/null 2>&1; then
  PYTHON=python
else
  echo "check.sh: Python 3.9 or newer is required." >&2
  exit 1
fi

if ! "$PYTHON" -c 'import sys; exit(0 if sys.version_info >= (3, 9) else 1)'; then
  echo "check.sh: Python 3.9 or newer is required." >&2
  exit 1
fi

banner "build"
if ! make -s all; then
  echo "check.sh: the build failed, so nothing else can run." >&2
  exit 1
fi

banner "$case_folder"
if ! "$PYTHON" tools/check_cases.py "$case_folder"; then
  failures=1
fi

banner "result"
if [[ "$failures" -eq 0 ]]; then
  if [[ "$case_folder" == "cases" ]]; then
    echo "All published checks passed."
  else
    echo "All cases in $case_folder passed."
  fi
else
  echo "Some checks in $case_folder failed. Scroll up for the first one." >&2
fi
exit "$failures"
