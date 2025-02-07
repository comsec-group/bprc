// SPDX-License-Identifier: GPL-3.0-only
/*
 * Investigate IBP Insertion
 */

#include <err.h>
#include <syscall.h>
#include <unistd.h>

#include "bp_tools.h"
#include "branch.h"
#include "memtools.h"
#include "stats.h"

#define RB_OFFSET 0xcc0
#define RB_STRIDE (4096 + 256) // reduce false positives on Gracemont
#include "rb_tools.h"

#define NUM_ROUNDS 100000
#define NOP_PREFIX 0x1000
#define MAX_NUM_HISTORY 194

typedef struct registers
{
    u64 src_ptr;     // code snip that contains the jump instruction
    u64 dst_ptr_ptr; // jump target

    u64 memory_ptr;
} registers_t;

typedef struct exp_params
{
    u64 train_src;
    u64 train_src_cfi_label;
    u64 train_src_size;
    u64 num_matching_history;
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
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"
    "clflush (%rbx)\n"
    "lfence\n"
    "train_src_label_jump:\n"
    "jmp *(%rbx)\n"
    "int3\n"
);
extern unsigned char train_src_label_call[];
mk_snip(train_src_call,
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"
    "clflush (%rbx)\n"
    "lfence\n"
    "train_src_label_call:\n"
    "call *(%rbx)\n"
    "ret\n"
    "int3\n"
);
extern unsigned char train_src_label_ret[];
mk_snip(train_src_ret,
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"
    "pushq (%rbx)\n"
    "clflush (%rsp)\n"
    "lfence\n"
    "train_src_label_ret:\n"
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

void run_snip_jump(void *v)
{
    struct registers *r = v;

    // clang-format off
    asm volatile(
        // RSB underflow
        ".rept 16\n"
        "lea 1f(%%rip), %%r8\n"
        "pushq  %%r8       \n"
        "ret\n"
        "1:\n"
        ".endr\n"

        // avoid creation of an RSB entry
        "lea 1f(%%rip), %%r8\n"
        "pushq %%r8\n"
        "jmp *%%rdx\n"
        "1:\n"
        :
        : "d"(r->src_ptr), "b"(r->dst_ptr_ptr), "c"(r->memory_ptr)
        : "r8", "memory", "flags");
    // clang-format on
}

void run_exp_once(run_params_t *params)
{
    // params
    registers_t *train = params->train;
    registers_t *victim = params->victim;

    rb_reset();
    // isolate the measurements from the setup
    for (volatile int i = 0; i < 100000; ++i)
        ;

    run_snip_jump(train);

    rb_flush();
    run_snip_jump(victim);

    // we need this to reduce false positives on alderlake
    for (volatile int i = 0; i < 10000; ++i)
        ;
    rb_reload();
}

const u64 dummy_memory;
u64 history[MAX_NUM_HISTORY + 1];
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
        victim_src_addr = victim_jmp_addr - (params->train_src_cfi_label - params->train_src);
    } while (map_code(&victim_src_snip, victim_src_addr, _ptr(params->train_src), params->train_src_size));

    // put the victim destination anywhere
    u64 victim_dst_addr;
    do
    {
        victim_dst_addr = rand47();
    } while (init_snip(&victim_dst_snip, victim_dst_addr, victim_dst));

    // create a victim gadget at a random but BTB predictable location (i.e. within 32bit)
    u64 victim_gadget_addr;
    do
    {
        victim_gadget_addr = ((victim_jmp_addr & 0x7FFFFF000000ul) | (rand47() & 0xFFFFFFul));
    } while (init_snip(&victim_gadget_snip, victim_gadget_addr, victim_gadget));

    // put training control flow instruction at an address aliasing with the victim
    u64 train_jmp_addr = (victim_jmp_addr & 0x40FFFFFFFFFFul) ^ 0x600000000000ul;
    u64 train_src_addr = train_jmp_addr - (params->train_src_cfi_label - params->train_src);
    if (map_code(&train_src_snip, train_src_addr, _ptr(params->train_src), params->train_src_size))
    {
        err = 1;
        goto cleanup_victim_gadget;
    }

    // put training destination such that it shares the lower bits with the victim gadget addr
    u64 train_dst_addr = (train_src_addr & 0x7FFFFF000000ul) | (victim_gadget_addr & 0xFFFFFFul);
    if (init_snip(&train_dst_snip, train_dst_addr, train_dst))
    {
        err = 1;
        goto cleanup_train_src;
    }

    int secret = rand() % (RB_SLOTS - 2);

    registers_t train;
    registers_t victim;

    // give the victim a random history
    victim.src_ptr = bhb_rand_nearjmp_chain(victim_src_addr, NOP_PREFIX, MAX_NUM_HISTORY, NULL, 0, history);
    victim.dst_ptr_ptr = _ul(&victim_dst_snip.ptr_u64); // set the target to wherever
    victim.memory_ptr = RB_PTR + secret * RB_STRIDE;

    // give the training a history that matches the params->num_matching_history last branches to the victim
    train.src_ptr = bhb_rand_nearjmp_chain(train_src_addr, NOP_PREFIX, MAX_NUM_HISTORY, history, params->num_matching_history, NULL);
    train.dst_ptr_ptr = _ul(&train_dst_snip.ptr_u64);
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

    // print statistics
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

    rb_init();

    INFO("finding leaks\n");

    // run the experiment for all types of instructions

    // with random history

    exp_params_t params_jump = {
        .train_src = _ul(train_src_jump__snip),
        .train_src_cfi_label = _ul(train_src_label_jump),
        .train_src_size = snip_sz(train_src_jump),
        .num_matching_history = 0,
    };
    evaluate_experiment(&params_jump, "rand_jump_");

    exp_params_t params_call = {
        .train_src = _ul(train_src_call__snip),
        .train_src_cfi_label = _ul(train_src_label_call),
        .train_src_size = snip_sz(train_src_call),
        .num_matching_history = 0,
    };
    evaluate_experiment(&params_call, "rand_call_");

    exp_params_t params_ret = {
        .train_src = _ul(train_src_ret__snip),
        .train_src_cfi_label = _ul(train_src_label_ret),
        .train_src_size = snip_sz(train_src_ret),
        .num_matching_history = 0,
    };
    evaluate_experiment(&params_ret, "rand_ret_");

    // with matching history

    params_jump.num_matching_history = MAX_NUM_HISTORY;
    evaluate_experiment(&params_jump, "match_jump_");

    params_call.num_matching_history = MAX_NUM_HISTORY;
    evaluate_experiment(&params_call, "match_call_");

    params_ret.num_matching_history = MAX_NUM_HISTORY;
    evaluate_experiment(&params_ret, "match_ret_");
}
