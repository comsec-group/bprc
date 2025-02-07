// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <sys/time.h>
static inline unsigned long get_ms(void) {
        static struct timeval tp;
        gettimeofday(&tp, 0);
        return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

/* Wait for all memory operations to complete before continuing. On AMD
 * processors, this also creates a new basic block. */
#define mb() asm("mfence" ::: "memory");

/* Wait for all memory loads to complete before continuing. This is also a
 * Speculation Barrier: speculative execution will stop here. However
 * instruction and data cache prefetchers can still continue to fetch. */
#define rmb() asm("lfence" ::: "memory");

/* Wait for all memory writes to complete before continuing. The weakest memory
 * barrier. */
#define wmb() asm("sfence" ::: "memory");
