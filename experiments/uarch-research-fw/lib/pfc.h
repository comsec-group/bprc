// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "compiler.h"
#include "lib.h"
#include "log.h"
#include <err.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <syscall.h>
#include <unistd.h>

#ifndef PFC_EXCLUDE_USER
#define PFC_EXCLUDE_USER 0
#endif

#ifndef PFC_EXCLUDE_KERNEL
#define PFC_EXCLUDE_KERNEL 0
#endif

#ifndef ECORE_QUIRK
#define ECORE_QUIRK 0
#endif

#define PFC_PREPARE(results)                                             \
    do                                                                   \
    {                                                                    \
        for (int p = 0; p < (sizeof(results) / sizeof(results[0])); ++p) \
        {                                                                \
            prepare_pmc(&results[p].pmc, &results[p].pfc);               \
        }                                                                \
        rmb();                                                           \
    } while (0);

#define PFC_START(results, measurement_index)                                                               \
    do                                                                                                      \
    {                                                                                                       \
        rmb();                                                                                              \
        _Pragma("clang loop unroll(full)") for (int p = 0; p < (sizeof(results) / sizeof(results[0])); ++p) \
        {                                                                                                   \
            results[p].starts[measurement_index] = read_prepared_pmc(&results[p].pmc);                      \
        }                                                                                                   \
        rmb();                                                                                              \
    } while (0);

#define PFC_END(results, measurement_index)                                                                 \
    do                                                                                                      \
    {                                                                                                       \
        rmb();                                                                                              \
        _Pragma("clang loop unroll(full)") for (int p = 0; p < (sizeof(results) / sizeof(results[0])); ++p) \
        {                                                                                                   \
            results[p].ends[measurement_index] = read_prepared_pmc(&results[p].pmc);                        \
        }                                                                                                   \
        rmb();                                                                                              \
    } while (0);

#define PFC_VALIDATE(results)                                            \
    do                                                                   \
    {                                                                    \
        for (int p = 0; p < (sizeof(results) / sizeof(results[0])); ++p) \
        {                                                                \
            if (!validate_pmc(&results[p].pmc, &results[p].pfc))         \
            {                                                            \
                ERROR("Detected changes to mmap of pfc %d\n", p);        \
            }                                                            \
        }                                                                \
    } while (0);

#define PFC_CLOSE(results)                                               \
    do                                                                   \
    {                                                                    \
        for (int p = 0; p < (sizeof(results) / sizeof(results[0])); ++p) \
        {                                                                \
            CLOSE_MEASURE_CTXT(&results[p]);                             \
        }                                                                \
    } while (0);

#define PREPARE_MEASURE_CTXT(ctxt, event, measurement_rounds) \
    do                                                        \
    {                                                         \
        alloc_measure_ctxt(ctxt, measurement_rounds);         \
        (ctxt)->name = #event;                                \
        (ctxt)->pfc.config = event;                           \
        if (pfc_init(&(ctxt)->pfc))                           \
        {                                                     \
            err(1, "pfc_init (are you root?)");               \
        }                                                     \
        (ctxt)->do_cleanup = 1;                               \
    } while (0)

#define REUSE_MEASURE_CTXT(dst_ctxt, src_ctxt, measurement_rounds) \
    do                                                             \
    {                                                              \
        alloc_measure_ctxt(dst_ctxt, measurement_rounds);          \
        (dst_ctxt)->name = (src_ctxt)->name;                       \
        (dst_ctxt)->pfc.config = (src_ctxt)->pfc.config;           \
        (dst_ctxt)->pfc = (src_ctxt)->pfc;                         \
        (dst_ctxt)->do_cleanup = 0;                                \
    } while (0)

#define CLOSE_MEASURE_CTXT(ctxt)       \
    do                                 \
    {                                  \
        free((ctxt)->starts);          \
        free((ctxt)->ends);            \
        if ((ctxt)->do_cleanup)        \
            pfc_dispose(&(ctxt)->pfc); \
    } while (0)

typedef struct pmc
{
    long mask;
    long offset;
    unsigned int index;
    __u32 pre_measure_lock;
} pmc_t;

typedef struct pfc
{
    long config;
    long config1;
    long config2;
    int fd;
    struct perf_event_mmap_page *buf;
} pfc_t;

typedef struct measure_ctxt
{
    const char *name;
    pfc_t pfc;
    int do_cleanup;
    pmc_t pmc;
    size_t total_rounds;
    unsigned long *starts;
    unsigned long *ends;
} measure_ctxt_t;

void alloc_measure_ctxt(measure_ctxt_t *ctxt, size_t measurement_rounds);

__always_inline void prepare_pmc(struct pmc *pmc, struct pfc *pfc)
{
    pmc->pre_measure_lock = pfc->buf->lock;
    rmb(); // barrier to ensure that we have finished reading the lock before loading the values
    pmc->mask = (1ul << pfc->buf->pmc_width) - 1;
    pmc->offset = pfc->buf->offset;
    pmc->index = pfc->buf->index - 1;
}

__always_inline unsigned long read_pmc(long ecx)
{
    unsigned hi, lo;

    asm volatile("rdpmc" : "=a"(lo), "=d"(hi) : "c"(ecx));
    return (((unsigned long)hi << 32) | lo);
}

__always_inline unsigned long read_prepared_pmc(struct pmc *pmc)
{
    return (read_pmc(pmc->index) + pmc->offset) & pmc->mask;
}

/**
 * if pmc is still valid => 1
 * else                  => 0
 */
__always_inline int validate_pmc(struct pmc *pmc, struct pfc *pfc)
{
    rmb(); // barrier to ensure that we have finished measuring before loading the lock

    // either the lock was not touched or the values are all the same (not technically thread safe this check here)
    return pmc->pre_measure_lock == pfc->buf->lock || (pmc->mask == (1ul << pfc->buf->pmc_width) - 1 &&
                                                       pmc->offset == pfc->buf->offset &&
                                                       pmc->index == pfc->buf->index - 1);
}

__always_inline int perf_event_open(struct perf_event_attr *attr,
                                    pid_t pid, int cpu, int group_fd,
                                    unsigned long flags)
{
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

__always_inline unsigned long eval_sum_diff(measure_ctxt_t *context)
{
    unsigned long sum = 0;
    for (int r = 0; r < context->total_rounds; ++r)
    {
        sum += context->ends[r] - context->starts[r];
    }
    return sum;
}

__always_inline double eval_avg_diff(measure_ctxt_t *context)
{
    return eval_sum_diff(context) / (double)context->total_rounds;
}

int _pfc_init(struct pfc *p, int no_user, int no_kernel, int ecore_quirk);

#define pfc_init(p) _pfc_init(p, PFC_EXCLUDE_USER, PFC_EXCLUDE_KERNEL, ECORE_QUIRK)

void pfc_dispose(struct pfc *p);
