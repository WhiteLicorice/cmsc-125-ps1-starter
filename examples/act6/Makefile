# CMSC 125 Activity 6 examples.
#
#   make          build the simulator
#   make check    run every case and compare its output
#   make clean    delete the built simulator
#
# The simulator is deterministic, so every comparison is exact. check prints
# one line for each case, then a count. It exits 0 only when every case matches.

CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Werror
SOURCES := sim.c sim_engine.c
CASES   := equal-q1 equal-q5 mixed-q5 arrivals-q20 io-alone switch-cost

all: sim

sim: $(SOURCES) sim_engine.h
	$(CC) $(CFLAGS) -o sim $(SOURCES)

check: sim
	@pass=0; fail=0; \
	for c in $(CASES); do \
	  if ./sim $$c | cmp -s - expected/$$c.txt; then \
	    echo "$$c: ok"; \
	    pass=$$((pass + 1)); \
	  else \
	    echo "$$c: FAIL"; \
	    ./sim $$c | diff -u expected/$$c.txt - | head -20; \
	    fail=$$((fail + 1)); \
	  fi; \
	done; \
	echo "$$pass of $$((pass + fail)) cases match"; \
	test $$fail -eq 0

clean:
	rm -f sim

.PHONY: all check clean
