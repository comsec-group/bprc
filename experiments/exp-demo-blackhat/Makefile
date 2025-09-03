# SPDX-License-Identifier: GPL-3.0-only
CC = clang

INC = -I../uarch-research-fw/lib/ -I../uarch-research-fw/
LIB = ../uarch-research-fw/lib/memtools.o ../uarch-research-fw/lib/branch.o
CORE ?= 2

CFLAGS  = -g -O2 -Wall -Wno-language-extension-token -Wno-unused-function -pedantic $(LIB) $(INC)

.PHONY: all clean run

../uarch-research-fw/lib/%.o:
	make -C ../uarch-research-fw/lib MARCH=$(MARCH)

all: $(LIB) attack

attack: kaslr.o

run: all
	taskset -c $(CORE) ./attack

clean:
	$(RM) attack kaslr.o
	$(RM) $(LIB)
