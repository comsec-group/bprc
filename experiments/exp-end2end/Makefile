# SPDX-License-Identifier: GPL-3.0-only
CC = clang

SRC = $(wildcard *.c)
BIN = $(patsubst %.c,%,$(SRC))
INC = -I../uarch-research-fw/lib/ -I../uarch-research-fw/
LIB = ../uarch-research-fw/lib/memtools.o ../uarch-research-fw/lib/stats.o ../uarch-research-fw/lib/branch.o ../uarch-research-fw/lib/pfc.o
CORE ?= 2
KASLR_OFFSET ?= 0x0

CFLAGS  = -g -O2 -Wall -Wno-language-extension-token -Wno-unused-function -pedantic $(LIB) $(INC)

.PHONY: all clean run

../uarch-research-fw/lib/%.o:
	make -C ../uarch-research-fw/lib MARCH=$(MARCH)

all: $(LIB) $(BIN)

run-kaslr: all
	taskset -c $(CORE) ./kaslr
run-shadow: all
	taskset -c $(CORE) ./attack 0 $(KASLR_OFFSET)
run-benchmark: all
	taskset -c $(CORE) ./attack 1 $(KASLR_OFFSET)

clean:
	$(RM) $(BIN)
	$(RM) $(LIB)