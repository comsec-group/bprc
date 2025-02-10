// SPDX-License-Identifier: GPL-3.0-only
/*
 * Test Setup
 */

#include "ap_ioctl.h"
#include "log.h"

#define RB_OFFSET 0xcc0
#include "rb_tools.h"

#define PFC_EXCLUDE_USER 1
#if defined(MARCH_ZEN4) || defined(MARCH_ZEN5)
#include "pfc_config/amd/pfc_zen4.h"
#elif defined(MARCH_GRACE)
#include "pfc_config/intel/pfc_grace.h"
#else
#include "pfc_config/intel/pfc_golden.h"
#endif
#include "pfc.h"

#define NUM_MEASUREMENTS 10000

#define PFC_COUNT 1
measure_ctxt_t results[PFC_COUNT];

void noinline super_function(void *c)
{
    if (c)
    {
        asm volatile(
            "lea 1f(%%rip), %%r8\n"
            "jmp *%%r8\n"
            "nop\n"
            "1:\n" ::: "r8");
    }
}

int main(int argc, char const *argv[])
{

    // test performance counter

    PREPARE_MEASURE_CTXT(&results[0], BR_INST_RETIRED__ALL_BRANCHES, NUM_MEASUREMENTS);

    for (int a = 0; a < NUM_MEASUREMENTS; ++a)
    {
        PFC_PREPARE(results);
        PFC_START(results, a);
        // clang-format off
        asm volatile(
            "syscall\n"::"a"(1024):
        );
        // clang-format on
        PFC_END(results, a);
        PFC_VALIDATE(results);
    }

    for (int p = 0; p < PFC_COUNT; ++p)
    {
        INFO("%-30s sum: %03lu, avg: %lf\n", results[p].name, eval_sum_diff(&results[p]), eval_avg_diff(&results[p]));
    }

    PFC_CLOSE(results);

    INFO("Performance counters: OK\n");

    // test reload buffer
    rb_init();
    
    // test no hit
    rb_reset();
    for (int i = 0; i < NUM_MEASUREMENTS; ++i)
    {
        rb_flush();
        rb_reload();
    }
    u64 res_nohit  = rb_hist[rb_max_index(rb_hist, RB_SLOTS - 1)];
    INFO("Reload buffer cleared: %05lu\n", res_nohit);
    fflush(stdout);

    if (res_nohit > NUM_MEASUREMENTS / 1000) {
        ERROR("reload buffer has too many false positives\n");
        exit(1);
    }

    // test hit
    rb_reset();
    for (int i = 0; i < NUM_MEASUREMENTS; ++i)
    {
        rb_flush();
        *(volatile u8 *)(RB_PTR + RB_OFFSET + RB_STRIDE * SECRET);
        rb_reload();
    }
    u64 res_hit  = rb_hist[rb_max_index(rb_hist, RB_SLOTS - 1)];
    INFO("Reload buffer accessed: %05lu\n", res_hit);
    fflush(stdout);

    if (res_hit < (NUM_MEASUREMENTS - (NUM_MEASUREMENTS / 1000))) {
        ERROR("reload buffer has too many false negatives\n");
        exit(1);
    }

    INFO("Reload buffer: OK\n");

    // test kernel module

    ap_init();
    super_function((void *)0);         // avoid kernel page faults
    ap_run(super_function, (void *)1); // try to run in kernel

    INFO("Kernel module ap: OK\n");

    return 0;
}
