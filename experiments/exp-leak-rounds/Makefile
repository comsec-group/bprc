# SPDX-License-Identifier: GPL-3.0-only
CC = clang

SRC = $(wildcard *.c)
BIN = $(patsubst %.c,%,$(SRC))
INC = -I../uarch-research-fw/lib/ -I../uarch-research-fw/kmod_ap/
LIB = ../uarch-research-fw/lib/memtools.o ../uarch-research-fw/lib/stats.o ../uarch-research-fw/lib/branch.o ../uarch-research-fw/lib/msr.o
CORE ?= 2
MARCH ?= MARCH_GOLDEN

CFLAGS  = -g -O2 -Wall -Wno-language-extension-token -Wno-unused-function -pedantic $(LIB) $(INC) -D$(MARCH)

.PHONY: all clean

../uarch-research-fw/lib/%.o:
	make -C ../uarch-research-fw/lib

all: $(LIB) $(BIN)

run: all
	taskset -c $(CORE) ./main

clean:
	$(RM) $(BIN)
	$(RM) $(LIB)