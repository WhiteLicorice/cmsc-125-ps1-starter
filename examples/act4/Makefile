# CMSC 125 Activity 4 examples.
#
#   make          build every example
#   make check    build every example, run it, and compare its output
#   make clean    delete the binaries and the files the examples write
#
# check merges each example's standard output and standard error, and compares
# the result against expected/<name>.txt. Every example is deterministic, so
# every comparison is exact. Each run is capped at ten seconds. Every example
# reaps its own children before it exits.
#
# Every example sets stdout unbuffered, so the order you see is the order the
# writes reached the pipe.

CC       := cc
CFLAGS   := -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L
EXAMPLES := fd_table redirect pipe_basic pipe_eof zombie orphan signal_kill
RUNTIME  := fd_table.txt redirect.out

all: $(EXAMPLES)

$(EXAMPLES): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

check: all
	@pass=0; fail=0; \
	for ex in $(EXAMPLES); do \
	  out=$$(timeout 10 ./$$ex 2>&1); rc=$$?; \
	  if [ $$rc -ne 0 ]; then \
	    echo "$$ex: FAIL (exit $$rc)"; \
	    fail=$$((fail + 1)); \
	    continue; \
	  fi; \
	  if [ "$$out" = "$$(cat expected/$$ex.txt)" ]; then \
	    echo "$$ex: ok"; \
	    pass=$$((pass + 1)); \
	  else \
	    echo "$$ex: FAIL (output differs)"; \
	    printf '%s\n' "$$out" | diff -u expected/$$ex.txt - | head -20; \
	    fail=$$((fail + 1)); \
	  fi; \
	done; \
	echo "$$pass of $$((pass + fail)) outputs match"; \
	test $$fail -eq 0

clean:
	rm -f $(EXAMPLES) $(RUNTIME)

.PHONY: all check clean
