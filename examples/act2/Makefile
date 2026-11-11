# CMSC 125 act2 examples.
#
#   make          build every example
#   make check    build every example, run it, and compare the output
#   make clean    delete the build output
#
# Every example is compiled with the same flags. The check compares output
# exactly when the program is deterministic, and by a stated property when it
# is not. It prints one line per example, then a line for the leftover-process
# check, then a count of the examples. It exits 0 only when every check passes.
#
# The buffering check needs `script` from util-linux, because it runs that
# example once on a terminal and once with the output redirected.

CC     := cc
CFLAGS := -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L
PROGS  := fork_basic fork_wait fork_copy fork_exec shell_loop buffering

.PHONY: all check clean

all: $(PROGS)

$(PROGS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGS) raw.txt got.txt exp.txt out.txt

check: all
	@set -u; \
	pass=0; fail=0; bad=0; \
	ok() { printf 'PASS %s\n' "$$1"; pass=$$((pass + 1)); }; \
	no() { printf 'FAIL %s: %s\n' "$$1" "$$2"; fail=$$((fail + 1)); }; \
	report() { if [ "$$1" = 1 ]; then ok "$$2"; else no "$$2" "$$3"; fi; }; \
	\
	verdict=1; \
	./fork_basic | sort > raw.txt; \
	sed 's/[0-9][0-9]*/N/g' raw.txt > got.txt; \
	printf 'child: pid N fork N\nparent: pid N fork N\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	awk '/^child:/{c=$$3; cf=$$5; nc++} \
	     /^parent:/{p=$$3; pf=$$5; np++} \
	     END{exit !((nc == 1) && (np == 1) && (cf == 0) && (pf == c) && (p != c))}' \
	    raw.txt || verdict=0; \
	report "$$verdict" fork_basic \
	  "the two lines or their return values do not match the fork contract"; \
	\
	verdict=1; \
	./fork_wait > raw.txt; \
	sed 's/[0-9][0-9]*/N/g' raw.txt > got.txt; \
	printf 'child: pid N\nparent: pid N waited for N\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	awk '/^child:/{c=$$3} /^parent:/{w=$$6; p=$$3} \
	     END{exit !((w == c) && (p != c))}' raw.txt || verdict=0; \
	report "$$verdict" fork_wait \
	  "the child line is not first, or wait did not return the child pid"; \
	\
	verdict=1; \
	./fork_copy > got.txt; \
	printf 'child: x = 400\nparent: x = 100\nparent: x = 300 after the change\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	report "$$verdict" fork_copy "the parent did not keep its own copy of x"; \
	\
	verdict=1; \
	./fork_exec > got.txt; \
	printf 'the child is now echo\nparent: the child finished\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	report "$$verdict" fork_exec "the child did not run echo, or the parent did not wait"; \
	\
	verdict=1; \
	printf 'echo one\necho two\n' | ./shell_loop > got.txt; \
	printf 'one\ntwo\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	report "$$verdict" shell_loop "the loop did not run both commands in order"; \
	\
	verdict=1; \
	./buffering > got.txt; \
	printf 'before fork\nchild\nbefore fork\nparent\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	./buffering flush > got.txt; \
	printf 'before fork\nchild\nparent\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	script -qc ./buffering /dev/null | tr -d '\r' > got.txt; \
	printf 'before fork\nchild\nparent\n' > exp.txt; \
	diff -u exp.txt got.txt > /dev/null || verdict=0; \
	report "$$verdict" buffering \
	  "the buffered copy of the line did not print twice with the output redirected"; \
	\
	leftover=$$(ps -e -o comm= | grep -c -E '^(fork_basic|fork_wait|fork_copy|fork_exec|shell_loop|buffering)$$' || true); \
	if [ "$$leftover" = 0 ]; then printf 'PASS %s\n' "no leftover process"; \
	else printf 'FAIL %s: %s still running\n' "no leftover process" "$$leftover"; bad=1; fi; \
	\
	printf '\n6 examples: %s passed, %s failed\n' "$$pass" "$$fail"; \
	test "$$fail" -eq 0 && test "$$bad" -eq 0
