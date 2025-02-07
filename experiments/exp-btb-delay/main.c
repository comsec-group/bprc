// SPDX-License-Identifier: GPL-3.0-only
/*
 * Measure Branch Predictor Delayed Update
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "compiler.h"
#include "memtools.h"
#include "stats.h"

// get the appropriate pfc configuration per microarchitecture
#define PFC_EXCLUDE_KERNEL 1
#if defined(MARCH_ZEN4) || defined(MARCH_ZEN5)
#include "pfc_config/amd/pfc_zen4.h"
#elif defined(MARCH_GRACE)
#include "pfc_config/intel/pfc_grace.h"
#elif defined(MARCH_ARMv8)
#include "pfc_config/arm/armv8.2-a.h"
#else
#include "pfc_config/intel/pfc_golden.h"
#endif
#if !defined(MARCH_ARMv8)
#include "pfc.h"
#endif

#if defined(MARCH_ARMv8)

#define DELAY_OP "nop\n"
#define DELAY_OP_SIZE 4

#define SYNC_OP "isb\n"

#else

// Alternative delay operations

// #define DELAY_OP "mov $0, %rax\ncpuid\n"
// #define DELAY_OP_SIZE 9

// #define DELAY_OP "lfence\n"
// #define DELAY_OP_SIZE 3

// #define DELAY_OP "pause\n"
// #define DELAY_OP_SIZE 2

#define DELAY_OP "nop\n"
#define DELAY_OP_SIZE 1

#define SYNC_OP "lfence\n"
#define SYNC_OP_SIZE 3
#endif

#define MAX_OPS 1024
#define NUM_ROUNDS 100000

#if defined(MARCH_ARMv8)
#define BHI_ENABLE_SYSCALL 450
#define JUMP_PFC_CONFIG BR_MIS_PRED_RETIRED
#define CALL_PFC_CONFIG BR_MIS_PRED_RETIRED
#define RET_PFC_CONFIG BR_MIS_PRED_RETIRED
#elif defined(MARCH_SKYLAKE)
// skylake CPUs don't have the more precise PMUs
#define JUMP_PFC_CONFIG BR_MISP_RETIRED__ALL_BRANCHES
#define CALL_PFC_CONFIG BR_MISP_RETIRED__ALL_BRANCHES
#define RET_PFC_CONFIG BR_MISP_RETIRED__ALL_BRANCHES
#else
#define JUMP_PFC_CONFIG BR_MISP_RETIRED__INDIRECT
#define CALL_PFC_CONFIG BR_MISP_RETIRED__INDIRECT
#define RET_PFC_CONFIG BR_MISP_RETIRED__RET
#endif

#if !defined(MARCH_ARMv8)
measure_ctxt_t pfc_results[1];
#endif
u64 results[NUM_ROUNDS];
u64 jmp_table[4];

typedef struct registers
{
    u64 start;
    u64 *jmp_table;
} registers_t;

typedef struct exp_params
{
    u64 train_src;
    u64 train_src_size;
    int is_call;

    int num_nops;
} exp_params_t;

typedef struct run_params
{
    registers_t reg_set;
    u64 attempt;
} run_params_t;

#if defined(MARCH_ARMv8)
// clang-format off
mk_snip(train_src_jump,
    ".rept " STR(MAX_OPS)"\n"
    DELAY_OP
    ".endr\n"
    SYNC_OP
    "add x0, x0, #8\n"
    "ldr x1, [x0]\n"
    "br x1\n"
);
mk_snip(train_src_call,
    ".rept " STR(MAX_OPS)"\n"
    DELAY_OP
    ".endr\n"
    SYNC_OP
    "add x0, x0, #8\n"
    "ldr x1, [x0]\n"
    "blr x1\n"
);
mk_snip(train_src_ret,
    ".rept " STR(MAX_OPS)"\n"
    DELAY_OP
    ".endr\n"
    SYNC_OP
    "add x0, x0, #8\n"
    "ldr lr, [x0]\n"
    "ret\n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    "mov lr, x4\n"
    "ret\n"
);
// clang-format on
#else
// clang-format off
mk_snip(train_src_jump,
    ".rept " STR(MAX_OPS)"\n"
    DELAY_OP
    ".endr\n"
    SYNC_OP
    "addq $8, %rsi\n"
    "jmp *(%rsi)\n"
    "int3\n"
);
mk_snip(train_src_call,
    ".rept " STR(MAX_OPS)"\n"
    DELAY_OP
    ".endr\n"
    SYNC_OP
    "addq $8, %rsi\n"
    "call *(%rsi)\n"
    "ret\n"
    "int3\n"
);
mk_snip(train_src_ret,
    ".rept " STR(MAX_OPS)"\n"
    DELAY_OP
    ".endr\n"
    SYNC_OP
    "addq $8, %rsi\n"
    "pushq (%rsi)\n"
    "ret\n"
    "int3\n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    "ret\n"
);
// clang-format on
#endif

noinline void run_exp_once(run_params_t *params)
{
    u64 pre, post;

#if defined(MARCH_ARMv8)
    // underflow the RSB to get other predictions
    asm volatile(
        ".rept 32\n"
        "adr lr, 1f\n"
        "ret\n"
        "1:\n"
        ".endr\n" ::: "lr");

    // start measurement
    asm volatile("isb");
    asm volatile("mrs %0, PMEVCNTR0_EL0" : "=r"(pre) : :);
    asm volatile("isb");

    register unsigned long x0 asm("x0") = _ul(params->reg_set.jmp_table - 1);
    register unsigned long x1 asm("x1") = params->reg_set.start;
    asm volatile(
        // reduces noise in the results
        ".rept 64\n"
        "isb\n"
        ".endr\n"
        // prepare experiment exit
        "adr x4, end_exp\n"
        "mov lr, x1\n"
        // appears to cause less noise in the performance counter than jmp or call on some cores
        "ret\n"
        "end_exp:\n"
        : "+r"(x0), "+r"(x1)::"lr", "x4");

    // stop measurement
    asm volatile("isb");
    asm volatile("mrs %0, PMEVCNTR0_EL0" : "=r"(post) : :);
    asm volatile("isb");
#else

    // underflow the RSB to get RRSBA predictions
    asm volatile(
        ".rept 32\n"
        "lea 1f(%%rip), %%r8\n"
        "pushq  %%r8       \n"
        "ret\n"
        "1:\n"
        ".endr\n" ::: "r8");

    // start measurement
    PFC_PREPARE(pfc_results);
    asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
    PFC_START(pfc_results, 0);
    asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");

#ifdef MARCH_ZEN4
    // avoid inserting RSB entries that cause noise in the results on Zen 4
    asm volatile(
        "lea 1f(%%rip), %%r11\n"
        "pushq %%r11\n"
        "jmp *(%%rax)\n"
        "1:\n"
        :
        : "a"(&params->reg_set.start), "S"(params->reg_set.jmp_table - 1)
        : "r11");
#else
    // avoid noise on Intel by having the RSB entry (is removed in ret experiment by first ret)
    asm volatile(
        "call *(%%rax)\n"
        :
        : "a"(&params->reg_set.start), "S"(params->reg_set.jmp_table - 1));
#endif

    // stop measurement
    PFC_PREPARE(pfc_results);
    asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
    PFC_END(pfc_results, 0);
    asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
    pre = pfc_results[0].starts[0];
    post = pfc_results[0].ends[0];
#endif

    results[params->attempt] = post - pre;
}

void run_exp(exp_params_t *params)
{
    int num_nops = params->num_nops;

    // build the set of code snippets in advance
    code_snip_t src_snip;
    code_snip_t dst_snip;

    for (int a = 0; a < NUM_ROUNDS; ++a)
    {
        u64 src_addr;
        do
        {
#if defined(MARCH_ARMv8)
            // Pixel 6 has limited virtual address space and needs instruction alignment
            src_addr = rand38() & (~0x3ul);
#else
            src_addr = rand47();
#if defined(MARCH_ZEN4)
            // ensure aliasing on AMD Zen 4
            src_addr &= ~(0x3Ful);
            if (params->is_call)
            {
                // cache line boundary inside the call instruction
                src_addr |= 0x37;
            }
            else
            {
                // cache line boundary between the last jump target and the jump
                src_addr -= SYNC_OP_SIZE;
            }
#endif
#endif
        } while (map_code(&src_snip, src_addr, _ptr(params->train_src), params->train_src_size));

        u64 dst_addr;
        do
        {
#if defined(MARCH_ARMv8)
            // Pixel 6 has limited virtual address space and needs instruction alignment
            dst_addr = rand38() & (~0x3ul);
#else
            dst_addr = rand47();
#endif
        } while (init_snip(&dst_snip, dst_addr, train_dst));

        i64 jmp_offset = (MAX_OPS - num_nops) * DELAY_OP_SIZE;

#if defined(MARCH_ZEN4) || defined(MARCH_ZEN5) || defined(MARCH_SKYLAKE)
        // ensure aliasing on AMD Zen 4 and AMD Zen 5
        u64 start = src_snip.ptr_u64 + jmp_offset;
#else
        // reduces noise on intel performance cores
        u64 start = src_snip.ptr_u64;
#endif

        int i = 0;

        // warmup reduces noise on intel performance cores
#if defined(MARCH_GOLDEN) || defined(MARCH_CYPRESS) || defined(MARCH_SKYLAKE)
        code_snip_t dst_warmup_snip;
        u64 dst_warmup_addr;
        do
        {
            dst_warmup_addr = rand47();
        } while (init_snip(&dst_warmup_snip, dst_warmup_addr, train_dst));

        run_params_t run_params_warmup = {
            .attempt = a,
            .reg_set = {.jmp_table = jmp_table, .start = src_snip.ptr_u64},
        };

        jmp_table[i++] = dst_warmup_addr;
        run_exp_once(&run_params_warmup);
        i = 0;
#endif

        run_params_t run_params = {
            .attempt = a,
            .reg_set = {.jmp_table = jmp_table, .start = start},
        };

        jmp_table[i++] = src_snip.ptr_u64 + jmp_offset;
        jmp_table[i++] = src_snip.ptr_u64 + jmp_offset;
        jmp_table[i++] = dst_addr;
        run_exp_once(&run_params);

#if defined(MARCH_GOLDEN) || defined(MARCH_CYPRESS) || defined(MARCH_SKYLAKE)
        junmap(&dst_warmup_snip);
#endif
        junmap(&dst_snip);
        junmap(&src_snip);

#if defined(MARCH_ARMv8)
        // on ARM we don't use the Linux perf so we need to account for context switches ourselves.
        // Since our code under test has only 5 branches, we retry any measurements that report
        // more misses than there are branches.
        if (results[a] > 5)
        {
            printf("we were switched out during critical section, retrying...\n");
            a -= 1;
        }
#endif
    }
}

double res_avg[MAX_OPS + 1] = {0};
void evaluate_experiment(exp_params_t *params, char *label_prefix, int pfc_index)
{
    printf("running %s\n", label_prefix);

    // initialize pfc
#if defined(MARCH_ARMv8)
    register long x0 asm("x0") = pfc_index | ARM_PMU_FILTER_EL1_MASK | ARM_PMU_FILTER_EL2_MASK | ARM_PMU_FILTER_EL3_MASK;
    asm volatile("msr PMEVTYPER0_EL0, x0" ::"r"(x0) :); // select the performance counter
#else
    PREPARE_MEASURE_CTXT(&pfc_results[0], pfc_index, 1);
#endif

    for (int num_nops = 0; num_nops <= MAX_OPS; ++num_nops)
    {
        params->num_nops = num_nops;

        run_exp(params);

        // we subtract the additional guaranteed misses from the total
#if defined(MARCH_SKYLAKE) || defined(MARCH_ARMv8)
        // subtract one more since these architectures count the returns with the other branches
        res_avg[num_nops] = stats_avg_u64(results, NUM_ROUNDS) - 3;
#else
        res_avg[num_nops] = stats_avg_u64(results, NUM_ROUNDS) - 2;
#endif

        printf("%snum_nops %03d: ", label_prefix, num_nops);
        printf("misp avg: %lf", res_avg[num_nops]);
        printf("\n");
        fflush(stdout);
    }

    // print results
    stats_results_start();
    printf("%s", label_prefix);
    stats_print_arr_f64("avg_mispredictions", res_avg, MAX_OPS + 1);
    stats_results_end();

#if !defined(MARCH_ARMv8)
    PFC_CLOSE(pfc_results);
#endif
}

int main(int argc, char *argv[])
{
    srandom(getpid());

#if defined(MARCH_ARMv8)
    // enable the performance counters in userspace
    register long syscall_number asm("w8") = BHI_ENABLE_SYSCALL;
    register long syscall_ret asm("x0");
    asm volatile("svc 0\n" : "=r"(syscall_ret) : "r"(syscall_number) :);
#endif

    exp_params_t params_jump = {
        .train_src = _ul(train_src_jump__snip),
        .train_src_size = snip_sz(train_src_jump),
        .is_call = 0,
    };
    evaluate_experiment(&params_jump, "jump_", JUMP_PFC_CONFIG);

    exp_params_t params_call = {
        .train_src = _ul(train_src_call__snip),
        .train_src_size = snip_sz(train_src_call),
        .is_call = 1,
    };
    evaluate_experiment(&params_call, "call_", CALL_PFC_CONFIG);

    // skylake CPUs don't have the ret PMU
#ifndef MARCH_SKYLAKE
    exp_params_t params_ret = {
        .train_src = _ul(train_src_ret__snip),
        .train_src_size = snip_sz(train_src_ret),
        .is_call = 0,
    };
    evaluate_experiment(&params_ret, "ret_", RET_PFC_CONFIG);
#endif
}
