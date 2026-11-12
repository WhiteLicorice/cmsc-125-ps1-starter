# Build and check the "Direct Execution, Traps, and Preemption" examples.
#
# This file is a user-level illustration of kernel mechanisms. The programs
# show a trap, its cost, and a timer, at user level. They do not implement
# the kernel.

CC       ?= cc
CFLAGS   := -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L
EXAMPLES := raw_syscall trap_cost timer_interrupt

.PHONY: all check clean

# Build every example.
all: $(EXAMPLES)

$(EXAMPLES): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Build, run, and compare every example. Exit 0 only if all pass.
check: all
	@fail=0; count=0; \
	report() { count=$$((count + 1)); \
		if [ "$$1" = ok ]; then echo "PASS $$2"; else echo "FAIL $$2"; fail=$$((fail + 1)); fi; }; \
	out=$$(./raw_syscall 2>&1); \
	exp=$$(printf 'limited direct execution\nlimited direct execution\nlibc write returned 25\nsyscall write returned 25'); \
	[ "$$out" = "$$exp" ] && report ok raw_syscall || report bad raw_syscall; \
	out=$$(./trap_cost 2>&1); \
	plain=$$(printf '%s\n' "$$out" | awk -F= '/^plain_call_ns=/{print $$2}'); \
	trap=$$(printf '%s\n' "$$out" | awk -F= '/^trap_call_ns=/{print $$2}'); \
	awk -v p="$$plain" -v t="$$trap" 'BEGIN{exit !(t > p)}' && report ok trap_cost || report bad trap_cost; \
	out=$$(./timer_interrupt 2>&1); \
	first=$$(printf '%s\n' "$$out" | sed -n '1p'); \
	iters=$$(printf '%s\n' "$$out" | sed -n 's/^loop iterations: //p'); \
	[ "$$first" = "timer fired 20 times" ] && [ -n "$$iters" ] && [ "$$iters" -gt 0 ] && report ok timer_interrupt || report bad timer_interrupt; \
	echo "$$count examples, $$((count - fail)) passed, $$fail failed"; \
	[ $$fail -eq 0 ]

# Remove the built programs.
clean:
	rm -f $(EXAMPLES)
