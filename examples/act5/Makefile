# Build and check the act5 examples.
#
# schedsim.c holds the model and the printer. demo_cases.c holds the
# student-facing cases. Each case prints one schedule and its metrics. make
# check compares the output of every case with the recorded output in
# expected/. This folder holds only the cases the students see.

CC       ?= cc
CFLAGS   := -std=c11 -Wall -Wextra -Werror
BIN      := schedsim
SOURCES  := schedsim.c demo_cases.c

CASES := fifo-basic fifo-convoy sjf-convoy fifo-arrivals sjf-arrivals \
         stcf-arrivals stcf-same

.PHONY: all check clean

# Build the simulator.
all: $(BIN)

$(BIN): $(SOURCES) schedsim.h
	$(CC) $(CFLAGS) -o $@ $(SOURCES)

# Run every case and compare it with its expected output.
# Exit 0 only when every case passes.
check: all
	@fail=0; count=0; \
	for c in $(CASES); do \
		count=$$((count + 1)); \
		if ./$(BIN) $$c | diff -u expected/$$c.txt - >/dev/null; then \
			echo "PASS $$c"; \
		else \
			echo "FAIL $$c"; fail=$$((fail + 1)); \
		fi; \
	done; \
	echo "$$count cases, $$((count - fail)) passed, $$fail failed"; \
	[ $$fail -eq 0 ]

# Remove the built program.
clean:
	rm -f $(BIN)
