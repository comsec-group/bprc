// SPDX-License-Identifier: GPL-3.0-only
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include "branch.h"

#define INT32_MAX 0x7fffffff
#define INT32_MIN (-0x7fffffff - 1)

// clang-format off
mk_snip(table_jumper,
    "addq $8, %rax\n"
    "jmp *-0x08(%rax)\n"
);
// clang-format on

void set_direct_jump_buf(void *buf, void *src_address, void *dst_address)
{
    // yes, this function is buggy, if the addresses are futher apart than int64_t can store

    u64 usrc = (u64)src_address;
    u64 udst = (u64)dst_address;

    i64 offset;
    offset = udst - usrc - sizeof(direct_near_jump_t);
    // printf("jump %p to %p by %d\n", src_address, dst_address, (int)offset);
    // fflush(stdout);
    assert(offset <= INT32_MAX && offset >= INT32_MIN);          // can only handle near jumps
    assert(offset >= 0 || offset < -sizeof(direct_near_jump_t)); // we don't want to jump into middle of the jump instruction

    store_direct_near_jump((direct_near_jump_t *)buf, offset);
}

void clear_jmps(u64 memory, size_t memory_size)
{
    // fill the memory with NOP operations first
    memset(_ptr(memory), OP_NOP, memory_size);
}

int bhb_rand_indjmp(bhb_indjmp_ctxt_t *ctxt, u64 target, size_t num_jumps)
{
    return bhb_rand_indjmp_chain(ctxt, target, num_jumps, NULL, 0);
}

int bhb_rand_indjmp_chain(bhb_indjmp_ctxt_t *ctxt, u64 target, size_t num_jumps, bhb_indjmp_ctxt_t *old_context, size_t num_matching_jumps)
{
    if (num_matching_jumps > num_jumps)
    {
        err(1, "num_matching_jumps > num_jumps");
    }

    ctxt->target = target;
    ctxt->allocated_prefix = 0;
    ctxt->num_jumps = num_jumps;
    ctxt->snips = NULL;
    ctxt->jump_table = NULL;

    ctxt->snips = calloc(num_jumps, sizeof(ctxt->snips[0]));
    if (!ctxt->snips)
    {
        bhb_rand_indjmp_close(ctxt);
        return 1;
    }
    ctxt->jump_table = calloc(num_jumps + 2, sizeof(ctxt->jump_table[0])); // +2 so we don't go out of bounds in the tramp with num_jumps = 0
    if (!ctxt->snips)
    {
        bhb_rand_indjmp_close(ctxt);
        return 2;
    }

    // start with new random jumps
    for (; ctxt->allocated_prefix < num_jumps - num_matching_jumps; ++ctxt->allocated_prefix)
    {
        if (init_snip_rand(&ctxt->snips[ctxt->allocated_prefix], ~0ul, table_jumper))
        {
            bhb_rand_indjmp_close(ctxt);
            return 3;
        }
        ctxt->jump_table[ctxt->allocated_prefix] = ctxt->snips[ctxt->allocated_prefix].ptr_u64;
    }

    // follow up with the matching jumps
    for (int i = 0; i < num_matching_jumps; ++i)
    {
        ctxt->jump_table[ctxt->allocated_prefix + i] = old_context->jump_table[old_context->num_jumps - num_matching_jumps + i];
    }

    // finish with the target
    ctxt->jump_table[num_jumps] = target;

    return 0;
}

void bhb_rand_indjmp_close(bhb_indjmp_ctxt_t *ctxt)
{
    --ctxt->allocated_prefix;
    for (; ctxt->allocated_prefix >= 0; --ctxt->allocated_prefix)
    {
        junmap(&ctxt->snips[ctxt->allocated_prefix]);
    }

    free(ctxt->jump_table);
    free(ctxt->snips);
}

u64 bhb_rand_nearjmp(u64 memory, size_t memory_size, size_t num_jumps)
{
    return bhb_rand_nearjmp_chain(memory, memory_size, num_jumps, NULL, 0, NULL);
}

u64 bhb_rand_nearjmp_chain(u64 memory, size_t memory_size, size_t num_jumps, u64 *jmp_old_chain, size_t num_matching_jumps, u64 *jmp_new_chain)
{
    return bhb_rand_nearjmp_chain_addr(memory, memory, memory_size, num_jumps, jmp_old_chain, num_matching_jumps, jmp_new_chain);
}

u64 bhb_rand_nearjmp_chain_addr(u64 real_addr, u64 memory, size_t memory_size, size_t num_jumps, u64 *jmp_old_chain, size_t num_matching_jumps, u64 *jmp_new_chain)
{
    // nothing to do here
    if (!num_jumps)
        return real_addr + memory_size;

    // require a minimum size of the region so we are sure to find enough space for all jumps
    if (memory_size < FALL_THROUGH_REGION_SIZE + 2 * (num_jumps - 1) * sizeof(direct_near_jump_t))
    {
        err(1, "refusing to place %zu jumps in %zu memory", num_jumps, memory_size);
    }

    // cannot handle too large memory right now
    if (memory_size > INT32_MAX / 2)
    {
        err(1, "cannot handle too large memory right now");
    }

    // cannot handle too large memory right now
    if (num_matching_jumps > num_jumps)
    {
        err(1, "cannot handle num_matching_jumps > num_jumps");
    }

    // array of nops for comparison
    direct_near_jump_t nops;
    memset(&nops, OP_NOP, sizeof(nops));

    // we start with selecting the fall-through location towards the end of the section
    u64 target_offset = memory_size - FALL_THROUGH_REGION_SIZE + (random() % FALL_THROUGH_REGION_SIZE);
    if (jmp_old_chain && num_matching_jumps > 0)
    {
        target_offset = jmp_old_chain[0];
    }

    // printf("mem start %p, mem end %p, fallthrough offset 0x%lx\n", _ptr(memory), _ptr(memory + memory_size), target_offset);
    // fflush(stdout);

    u64 jump_addr_offset = target_offset;

    // can't have a jump in the fallthrough region
    memory_size -= FALL_THROUGH_REGION_SIZE;

    // build the jump chain back to front because we need to know the next target
    int i = 0;
    for (; i < num_jumps; ++i)
    {
        if (jmp_new_chain)
        {
            jmp_new_chain[i] = target_offset;
        }

        if (jmp_old_chain && num_matching_jumps >= i + 1)
        {
            // select the old jump location as the next jump address
            jump_addr_offset = jmp_old_chain[i + 1];
        }
        else
        {
            // choose a random location that is not already used
            do
            {
                jump_addr_offset = (random() % (memory_size - sizeof(direct_near_jump_t)));
            } while (0 != memcmp(_ptr(memory + jump_addr_offset), &nops, sizeof(direct_near_jump_t)));
        }

        // place a jump to the previous target
        assert(jump_addr_offset >= 0);
        assert(jump_addr_offset + sizeof(direct_near_jump_t) < memory_size);
        // printf("src offset 0x%lx, dst offset 0x%lx\n", jump_addr_offset, target_offset);
        // fflush(stdout);
        set_direct_jump_buf(_ptr(memory + jump_addr_offset), _ptr(real_addr + jump_addr_offset), _ptr(real_addr + target_offset));

        target_offset = jump_addr_offset;
    }
    if (jmp_new_chain)
    {
        jmp_new_chain[i] = target_offset;
    }

    return real_addr + jump_addr_offset;
}
