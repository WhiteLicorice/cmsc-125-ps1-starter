# CMSC 125 multilevel feedback queue examples.
#
#   make          build the simulator
#   make check    build the simulator, run every case, and compare its output
#   make clean    delete the simulator
#
# check compares each case's output against expected/<case>.txt. The simulator
# is deterministic, so every comparison is exact. Each run prints the schedule
# and the metrics for one workload.

CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror
CASES   := learning gaming-old gaming-new boost-off boost-on

all: mlfq

mlfq: mlfq.c
	$(CC) $(CFLAGS) -o $@ $<

check: mlfq
	@pass=0; fail=0; \
	for case in $(CASES); do \
	  ./mlfq --case $$case > mlfq.tmp.out; \
	  if [ "$$(cat expected/$$case.txt)" = "$$(cat mlfq.tmp.out)" ]; then \
	    echo "$$case: ok"; \
	    pass=$$((pass + 1)); \
	  else \
	    echo "$$case: FAIL (output differs)"; \
	    diff -u expected/$$case.txt mlfq.tmp.out | head -20; \
	    fail=$$((fail + 1)); \
	  fi; \
	done; \
	rm -f mlfq.tmp.out; \
	echo "$$pass of $$((pass + fail)) outputs match"; \
	test $$fail -eq 0

clean:
	rm -f mlfq

.PHONY: all check clean
