// SPDX-License-Identifier: GPL-3.0-only
/*
 * Investigate BPI reliability
 */

#include <err.h>
#include <syscall.h>
#include <unistd.h>

#include "ap_ioctl.h"
#include "bp_tools.h"
#include "memtools.h"
#include "msr.h"
#include "stats.h"

#define RB_OFFSET 0xcc0
#define RB_STRIDE (4096 + 256) // reduce false positives on Gracemont
#include "rb_tools.h"

#define NUM_ROUNDS 100000
#define MAX_ATTEMPTS 32

u64 rb_hist_map[MAX_ATTEMPTS][RB_SLOTS];

typedef struct registers
{
    u64 src_ptr;     // code snip that contains the jump instruction
    u64 dst_ptr_ptr; // jump target

    u64 memory_ptr;
} registers_t;

typedef struct exp_params
{
    u64 src;
    u64 src_size;
} exp_params_t;

typedef struct run_params
{
    exp_params_t *exp_params;
    registers_t *train;
    registers_t *victim;
} run_params_t;

typedef struct exp_res
{
    u64 hits[MAX_ATTEMPTS];
    u64 check;
} exp_res_t;

// clang-format off
extern unsigned char train_src_label_jump[];
mk_snip(train_src_jump,
    BHB_SETUP(194, 0x10 - 2)
    "clflush (%rbx)\n"
    "lfence\n"
    "train_src_label_jump:\n"
    "jmp *(%rbx)\n"
    "int3\n"
);
extern unsigned char train_src_label_call[];
mk_snip(train_src_call,
    BHB_SETUP(194, 0x10 - 2)
    "clflush (%rbx)\n"
    "lfence\n"
    "train_src_label_call:\n"
    "call *(%rbx)\n"
    "ret\n"
    "int3\n"
);
extern unsigned char train_src_label_ret[];
mk_snip(train_src_ret,
    ".rept 16\n"
    "lea 1f(%rip), %r8\n"
    "pushq  %r8       \n"
    "ret\n"
    "1:\n"
    ".endr\n"

    // put in some distance between the underflow and the victim to avoid tainting the BTB
    ".rept 32\n"
    "nop\n"
    ".endr\n"

    "pushq (%rbx)\n"
    BHB_SETUP(194, 0x10 - 2)
    "clflush (%rsp)\n"
    "lfence\n"
    "train_src_label_ret:\n"
    "ret\n"
    "int3\n"
);
// clang-format on

// clang-format off
mk_snip(victim_dst,
    "lfence\n"
    "ret\n"
    "int3       \n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    "add $"STR(RB_OFFSET)", %rcx\n"
    "mov (%rcx), %rcx\n"
    "syscall\n"
    ".rept 32\n"
    "nop\n"
    ".endr\n"
    "ret\n"
    "int3       \n"
);
// clang-format on

void run_snip_call(void *v)
{
    struct registers *r = v;

    // clang-format off
    asm volatile(
        "call *%%rdx\n"
        :
        : "d"(r->src_ptr), "b"(r->dst_ptr_ptr), "c"(r->memory_ptr)
        : "r8", "memory", "flags");
    // clang-format on
}

// outside the function to make sure compiler does not optimize it away
struct ap_payload p;
void run_exp_once(run_params_t *params)
{
    // params
    registers_t *train = params->train;
    registers_t *victim = params->victim;

    p.fptr = run_snip_call;
    p.data = victim;

    // reset reload buffer histories
    memset(rb_hist_map, 0, sizeof(rb_hist_map));
    // isolate the measurements from the setup
    for (volatile int i = 0; i < 100000; ++i)
        ;

    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        rb_flush();

        // clang-format off
        asm volatile(
            "lea 1f(%%rip), %%r11\n"
            "pushq %%r11\n"
            "jmp *%0\n"
            "1:\n"
            :
            : "r"(train->src_ptr), "b"(train->dst_ptr_ptr), "c"(train->memory_ptr),
              "a"(SYS_ioctl), "D"(fd_ap), "S"(AP_IOCTL_RUN), "d"(&p) // ioctl params to run victim
            : "r8", "r11", "memory");
        // clang-format on

        // we need this to reduce false positives on alderlake
        for (volatile int i = 0; i < 10000; ++i)
            ;
        reload_range(RB_PTR + RB_OFFSET, RB_STRIDE, RB_SLOTS, rb_hist_map[attempt]);
    }
}

const u64 dummy_memory;
int run_exp(exp_params_t *params, exp_res_t *exp_res)
{
    int err = 0;

    // build the set of code snippets in advance
    code_snip_t train_src_snip;
    code_snip_t train_dst_snip;
    code_snip_t victim_dst_snip;

    // random victim target
    u64 victim_dst_addr;
    do
    {
        victim_dst_addr = rand47();
    } while (init_snip(&victim_dst_snip, victim_dst_addr, victim_dst));

    // random victim source address
    u64 train_src_addr = rand47();
    if (map_code(&train_src_snip, train_src_addr, _ptr(params->src), params->src_size))
    {
        err = 1;
        goto cleanup_victim_gadget;
    }

    // random but BTB predictable training target
    u64 train_dst_addr = (train_src_addr & 0x7FFFFF000000ul) | (rand47() & 0xFFFFFFul);
    if (init_snip(&train_dst_snip, train_dst_addr, train_dst))
    {
        err = 1;
        goto cleanup_train_src;
    }

    int secret = rand() % (RB_SLOTS - 2);

    registers_t train;
    registers_t victim;

    victim.src_ptr = train_src_addr;
    victim.dst_ptr_ptr = _ul(&victim_dst_snip.ptr_u64);
    victim.memory_ptr = RB_PTR + secret * RB_STRIDE;

    train.src_ptr = train_src_addr;
    train.dst_ptr_ptr = _ul(&train_dst_snip.ptr_u64);
    train.memory_ptr = _ul(&dummy_memory) - RB_OFFSET;

    run_params_t run_params = {
        .exp_params = params,
        .train = &train,
        .victim = &victim,
    };

    run_exp_once(&run_params);

    size_t check_secret = (secret - 1) < 0 ? (RB_SLOTS - 1) : (secret - 1);
    exp_res->check = 0;
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        exp_res->hits[attempt] = rb_hist_map[attempt][secret];
        exp_res->check += rb_hist_map[attempt][check_secret];
    }

    junmap(&train_dst_snip);
cleanup_train_src:
    junmap(&train_src_snip);
cleanup_victim_gadget:
    junmap(&victim_dst_snip);

    return err;
}

u64 results_attempts[MAX_ATTEMPTS] = {0};
u64 results[NUM_ROUNDS] = {0};
void evaluate_experiment(exp_params_t *params, char *label_prefix)
{
    for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
    {
        results_attempts[attempt] = 0;
    }

    INFO("running %s\n", label_prefix);
    u64 check = 0; // count false positives
    for (int r = 0; r < NUM_ROUNDS; ++r)
    {
        exp_res_t exp_res;

        if (run_exp(params, &exp_res))
        {
            // retry if there was an issue (i.e. addr generation)
            --r;
            continue;
        }

        results[r] = stats_sum_u64(exp_res.hits, MAX_ATTEMPTS);
        for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
        {
            results_attempts[attempt] += exp_res.hits[attempt];
        }
        check += exp_res.check;
    }

    // print statistics
    stats_results_start();
    printf("%s", label_prefix);
    stats_print_arr_u64("results_btb_attempts", results_attempts, MAX_ATTEMPTS);
    printf("%s", label_prefix);
    stats_print_u64("check", check);
    stats_results_end();
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    srandom(getpid());

    msr_init();
    rb_init();
    ap_init();

    INFO("finding leaks\n");

    // run the experiment for all types of instructions with IBRS=1, BHI_DIS_S=0
    msr_params_t msr_params = {
        .msr_nr = MSR_IA32_SPEC_CTRL,
        .msr_value = IBRS_MASK,
    };
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);

    exp_params_t params_jump = {
        .src = _ul(train_src_jump__snip),
        .src_size = snip_sz(train_src_jump),
    };
    evaluate_experiment(&params_jump, "jump_");

    // exp_params_t params_call = {
    //     .src = _ul(train_src_call__snip),
    //     .src_size = snip_sz(train_src_call),
    // };
    // evaluate_experiment(&params_call, "call_");

    // exp_params_t params_ret = {
    //     .src = _ul(train_src_ret__snip),
    //     .src_size = snip_sz(train_src_ret),
    // };
    // evaluate_experiment(&params_ret, "ret_");

    // run the experiment for all types of instructions with IBRS=1,  BHI_DIS_S=1
    msr_params.msr_value = BHI_DIS_S_MASK;
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);

    evaluate_experiment(&params_jump, "dis_jump_");
    // evaluate_experiment(&params_call, "dis_call_");
    // evaluate_experiment(&params_ret, "dis_ret_");
}
