// SPDX-License-Identifier: GPL-3.0-only
/*
 * Investigate `BHI_DIS_S`
 */

#include <err.h>
#include <syscall.h>
#include <unistd.h>

#include "ap_ioctl.h"
#include "branch.h"
#include "memtools.h"
#include "msr.h"
#include "stats.h"

#define PFC_EXCLUDE_USER 1
#ifdef MARCH_GRACE
#include "pfc_config/intel/pfc_grace.h"
#else
#include "pfc_config/intel/pfc_golden.h"
#endif
#include "pfc.h"

#define NUM_ROUNDS 100000
#define MAX_ATTEMPTS 8
#define NOP_PREFIX 0x1000
#define MAX_NUM_HISTORY 194

#define RESULT_INDEX_MISP_IND_CALL 0
#define RESULT_INDEX_MISP_IND_JMP 1
#define RESULT_INDEX_MISP_IND_RET 2
#define PFC_COUNT 3

measure_ctxt_t results[PFC_COUNT];

typedef struct registers
{
    u64 src_ptr;     // code snip that contains the jump instruction
    u64 dst_ptr_ptr; // jump target
} registers_t;

typedef struct exp_params
{
    void (*run_cmd)(void *);

    u64 src;
    u64 src_size;
} exp_params_t;

typedef struct run_params
{
    exp_params_t *exp_params;
    registers_t *test_code;
} run_params_t;

// clang-format off
mk_snip(src_jump,
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"

    ".rept 32\n"
    "nop\n"
    ".endr\n"

    "train_src_label_jump:\n"
    "jmp *(%rbx)\n"
    "int3\n"
);
mk_snip(src_call,
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"

    ".rept 32\n"
    "nop\n"
    ".endr\n"

    "train_src_label_call:\n"
    "call *(%rbx)\n"
    "ret\n"
    "int3\n"
);
mk_snip(src_ret,
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"

    ".rept 32\n"
    "nop\n"
    ".endr\n"

    "pushq (%rbx)\n"
    "train_src_label_ret:\n"
    "ret\n"
    "int3\n"
);
mk_snip(dst,
    "ret\n"
    "int3\n"
);
// clang-format on

void run_snip_call(void *v)
{
    struct registers *r = v;

    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {

        // start measurement
        PFC_PREPARE(results);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
        PFC_START(results, attempt);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");

        // clang-format off
        asm volatile(
            "call *%%rdx\n"
            :
            : "d"(r->src_ptr), "b"(r->dst_ptr_ptr)
            : "r8", "memory", "flags");
        // clang-format on

        // stop measurement
        PFC_PREPARE(results);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
        PFC_END(results, attempt);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
    }
}

void run_snip_jump(void *v)
{
    struct registers *r = v;

    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        // RSB underflow
        // clang-format off
        asm volatile(
            ".rept 32\n"
            "lea 1f(%rip), %r8\n"
            "pushq  %r8       \n"
            "ret\n"
            "1:\n"
            ".endr\n"
        );
        // clang-format on

        // start measurement
        PFC_PREPARE(results);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
        PFC_START(results, attempt);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");

        // clang-format off
        asm volatile(
            "lea 1f(%%rip), %%r8\n"
            "pushq %%r8\n"
            "jmp *%%rdx\n"
            "1:\n"
            :
            : "d"(r->src_ptr), "b"(r->dst_ptr_ptr)
            : "r8", "memory", "flags");
        // clang-format on

        // stop measurement
        PFC_PREPARE(results);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
        PFC_END(results, attempt);
        asm volatile("mov $0, %%rax\ncpuid" ::: "rax", "rbx", "rcx", "rdx");
    }
}

void run_exp(exp_params_t *params)
{
    code_snip_t src_snip;
    code_snip_t dst_snip;

    // put the branch source at a random memorylocation
    u64 src_addr;
    do
    {
        src_addr = rand47();
    } while (map_code(&src_snip, src_addr, _ptr(params->src), params->src_size));

    // put the branch destination at a random memory location that is far away from the source to avoid BTB hist
    while (init_snip(&dst_snip, (rand47() & 0x3FFFFFFFFFFFul) | (src_addr ^ 0x400000000000ul), dst))
        ;

    registers_t test_code;

    // generate a random history that will remain consistent during the experiment
    test_code.src_ptr = bhb_rand_nearjmp(src_addr, NOP_PREFIX, MAX_NUM_HISTORY);
    test_code.dst_ptr_ptr = _ul(&dst_snip.ptr_u64);

    run_params_t run_params = {
        .exp_params = params,
        .test_code = &test_code,
    };

    // run the code under test in kernel mode
    void (*run_cmd)(void *) = params->run_cmd;
    ap_run(run_cmd, &test_code);

    junmap(&src_snip);
    junmap(&dst_snip);
}

u64 diffs[PFC_COUNT][NUM_ROUNDS];
void evaluate_experiment(exp_params_t *params, char *label_prefix, int pfc_index)
{
    INFO("running %s\n", label_prefix);

    for (int r = 0; r < NUM_ROUNDS; ++r)
    {
        run_exp(params);

        for (u64 p = 0; p < PFC_COUNT; ++p)
        {
            // in this experiment we are only interested in the mistpredictions in the last run
            diffs[p][r] = results[p].ends[MAX_ATTEMPTS - 1] - results[p].starts[MAX_ATTEMPTS - 1];
        }
    }

    // calculate statistics
    u64 res_median = stats_median_u64(diffs[pfc_index], NUM_ROUNDS);
    f64 res_avg = stats_avg_u64(diffs[pfc_index], NUM_ROUNDS);
    f64 res_avg_call = stats_avg_u64(diffs[RESULT_INDEX_MISP_IND_CALL], NUM_ROUNDS);

    // print results
    stats_results_start();
    printf("%s_", label_prefix);
    stats_print_u64("median", res_median);
    printf("%s_", label_prefix);
    stats_print_f64("avg", res_avg);
    printf("%s_", label_prefix);
    stats_print_f64("avg_call", res_avg_call);
    stats_results_end();
}

int main(int argc, char *argv[])
{
    srandom(getpid());

    msr_init();
    ap_init();

    // initialize performance counters
    PREPARE_MEASURE_CTXT(&results[RESULT_INDEX_MISP_IND_CALL], BR_MISP_RETIRED__INDIRECT_CALL, MAX_ATTEMPTS);
    PREPARE_MEASURE_CTXT(&results[RESULT_INDEX_MISP_IND_JMP], BR_MISP_RETIRED__INDIRECT, MAX_ATTEMPTS);
    PREPARE_MEASURE_CTXT(&results[RESULT_INDEX_MISP_IND_RET], BR_MISP_RETIRED__RET, MAX_ATTEMPTS);

    // to ensure there is no page fault in supervisor mode due to measurements
    PFC_PREPARE(results);
    PFC_START(results, 0);
    PFC_END(results, 0);

    // run the experiment for all types of instructions with BHI_DIS_S=0
    msr_params_t msr_params = {
        .msr_nr = MSR_IA32_SPEC_CTRL,
        .msr_value = 0,
    };
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);

    exp_params_t params_jump = {
        .run_cmd = run_snip_call,
        .src = _ul(src_jump__snip),
        .src_size = snip_sz(src_jump),
    };
    evaluate_experiment(&params_jump, "jump", RESULT_INDEX_MISP_IND_JMP);

    exp_params_t params_call = {
        .run_cmd = run_snip_jump,
        .src = _ul(src_call__snip),
        .src_size = snip_sz(src_call),
    };
    evaluate_experiment(&params_call, "call", RESULT_INDEX_MISP_IND_CALL);

    exp_params_t params_ret = {
        .run_cmd = run_snip_jump,
        .src = _ul(src_ret__snip),
        .src_size = snip_sz(src_ret),
    };
    evaluate_experiment(&params_ret, "ret", RESULT_INDEX_MISP_IND_RET);

    // run the experiment for all types of instructions with BHI_DIS_S=1
    msr_params.msr_value = BHI_DIS_S_MASK;
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);

    evaluate_experiment(&params_jump, "dis_jump", RESULT_INDEX_MISP_IND_JMP);

    evaluate_experiment(&params_call, "dis_call", RESULT_INDEX_MISP_IND_CALL);

    evaluate_experiment(&params_ret, "dis_ret", RESULT_INDEX_MISP_IND_RET);

    PFC_CLOSE(results);
}
