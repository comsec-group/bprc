# SPDX-License-Identifier: GPL-3.0-only
CC ?= clang

all: memtools.o pfc.o branch.o stats.o msr.o

memtools.o: memtools.c memtools.h
	$(CC) -O2 -g -c memtools.c

pfc.o: pfc.c pfc.h
	$(CC) -O2 -g -c pfc.c

branch.o: branch.c branch.h
	$(CC) -O2 -g -c branch.c

stats.o: stats.c stats.h
	$(CC) -O2 -g -c stats.c

msr.o: msr.c msr.h
	$(CC) -O2 -g -c msr.c

.PHONY: clean
clean:
	-rm -fr *.o

