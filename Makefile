# Makefile: the starter's build. Do not edit.
#
#   make        build build/cmsc-125-ps1
#   make check  build, then run the public corpus
#   make clean  remove the build directory

CC ?= cc
CFLAGS ?= -std=c17 -O2 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L

BIN := build/cmsc-125-ps1
SRC := $(wildcard src/*.c)
HDR := $(wildcard include/*.h)

.PHONY: all check clean

all: $(BIN)

$(BIN): $(SRC) $(HDR)
	@mkdir -p build
	$(CC) $(CFLAGS) -Iinclude -o $(BIN) $(SRC)

check: $(BIN)
	./check.sh

clean:
	rm -rf build
