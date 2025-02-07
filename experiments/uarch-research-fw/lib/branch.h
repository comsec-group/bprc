// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "compiler.h"
#include "memtools.h"
#include "bp_tools.h"

/** according to Half&Half only the last 6 bits of the target address are used in branch history */
// #define FALL_THROUGH_REGION_SIZE (1 << 10)
#define FALL_THROUGH_REGION_SIZE (1 << 6)

// opcodes
#define OP_JMP_NEAR 0xE9
#define OP_RET_FAR 0xCB
#define OP_NOP 0x90

typedef struct direct_near_jump
{
    unsigned char opcode;
    unsigned char dest_byte_0;
    unsigned char dest_byte_1;
    unsigned char dest_byte_2;
    unsigned char dest_byte_3;

} direct_near_jump_t;

typedef struct bhb_indjmp_ctxt
{
    u64 entry;

    u64 target;
    i64 allocated_prefix;
    u64 num_jumps;

    code_snip_t *snips;
    u64 *jump_table;
} bhb_indjmp_ctxt_t;

void inline store_direct_near_jump(direct_near_jump_t *jump, i64 offset)
{
    jump->opcode = OP_JMP_NEAR;
    jump->dest_byte_0 = offset & 0xFF;
    jump->dest_byte_1 = (offset >> 8) & 0xFF;
    jump->dest_byte_2 = (offset >> 16) & 0xFF;
    jump->dest_byte_3 = (offset >> 24) & 0xFF;
}

void inline set_far_return(void *src_address)
{
    unsigned char *src = src_address;
    *src = OP_RET_FAR;
}

void inline set_nop(void *src_address)
{
    unsigned char *src = src_address;
    *src = OP_NOP;
}

/**
 * Set the memory operations to nop
 */
void clear_jmps(u64 memory, size_t memory_size);

/**
 * build a randomized indirect jump chain, clobbers rax
 *
 * @param ctxt       will hold the jump chain data
 * @param target     where the jump chain should jump to in the last jump
 * @param num_jumps  number of random jumps to add to the chain
 */
int bhb_rand_indjmp(bhb_indjmp_ctxt_t *ctxt, u64 target, size_t num_jumps);

/**
 * build a randomized indirect jump chain, clobbers rax
 *
 * @param ctxt                will hold the jump chain data
 * @param target              where the jump chain should jump to in the last jump
 * @param num_jumps           number of random jumps to add to the chain
 * @param old_context         an existing jump chain; used with num_matching_jumps (size must be at least num_matching_jumps)
 * @param num_matching_jumps  the number of jumps towards the end of the chain that should match old_context
 */
int bhb_rand_indjmp_chain(bhb_indjmp_ctxt_t *ctxt, u64 target, size_t num_jumps, bhb_indjmp_ctxt_t *old_context, size_t num_matching_jumps);

/**
 * build a randomized indirect jump chain, clobbers rax
 *
 * @param ctxt                will hold the jump chain data
 * @param target              where the jump chain should jump to in the last jump
 * @param num_jumps           number of random jumps to add to the chain
 * @param old_context         an existing jump chain; used with num_matching_jumps (size must be at least num_matching_jumps)
 * @param num_matching_jumps  the number of jumps towards the end of the chain that should match old_context
 */
int bhb_indjmp_chain(bhb_indjmp_ctxt_t *ctxt, u64 target, size_t num_jumps, bhb_indjmp_ctxt_t *old_context, size_t num_matching_jumps);

/**
 * execute the indirect jump chain
 *
 * @param ctxt  an initialized indirect jump context
 * @param rsi   value for the rsi register that is set before the jump chain is executed
 *
 */
__always_inline void bhb_rand_indjmp_tramp(bhb_indjmp_ctxt_t *ctxt, u64 rsi)
{

    // clang-format off
    asm volatile(
        "lea 1f(%%rip), %%r8\n"
        IRET(rcx, %)
        "1:"
        :
        : "c"(ctxt->jump_table[0]), "D"(&ctxt->jump_table[1]), "S"(rsi)
        : "rax", "r8", "flags"
    );
    // clang-format on
    // otherwise gcc does not seem to realize that rsi and rdi might have changed
    asm volatile("xor %%rsi, %%rsi" ::: "rsi");
    asm volatile("xor %%rdi, %%rdi" ::: "rdi");
}

/**
 * free resources associated with this indirect jump context
 */
void bhb_rand_indjmp_close(bhb_indjmp_ctxt_t *ctxt);

/**
 * Insert n random jumps that will fall through at the end of the section.
 * (potentially jumps to the instruction right after the given memory)
 *
 * @param memory              address of the memory region to modify
 * @param memory_size         size of the memory region to modify
 * @param num_jumps           number of random jumps to add to the chain
 *
 * @returns the location of the first jump (jump here to start the chain)
 */
u64 bhb_rand_nearjmp(u64 memory, size_t memory_size, size_t num_jumps);

/**
 * Insert n random jumps that will fall through at the end of the section.
 * (potentially jumps to the instruction right after the given memory)
 *
 * This extended function can also use an existing chain. That is useful
 * for partially matching branch histories.
 *
 * It can also return the chain that was created in this function.
 *
 * @param memory              address of the memory region to modify
 * @param memory_size         size of the memory region to modify
 * @param num_jumps           number of random jumps to add to the chain
 * @param jmp_old_chain       an existing jump chain; used with num_matching_jumps (size must be at least num_matching_jumps) (may be set to NULL)
 * @param num_matching_jumps  the number of jumps towards the end of the chain that should match jmp_old_chain
 * @param jmp_new_chain       a list of size at least num_jumps (may be set to NULL)
 *
 * @returns the location of the first jump (jump here to start the chain)
 */
u64 bhb_rand_nearjmp_chain(u64 memory, size_t memory_size, size_t num_jumps, u64 *jmp_old_chain, size_t num_matching_jumps, u64 *jmp_new_chain);

/**
 * Insert n random jumps that will fall through at the end of the section.
 * (potentially jumps to the instruction right after the given memory)
 *
 * This extended function can also use an existing chain. That is useful
 * for partially matching branch histories.
 *
 * It can also return the chain that was created in this function.
 *
 * @param addr                the address where the memory will be copied later
 * @param memory              address of the memory region to modify
 * @param memory_size         size of the memory region to modify
 * @param num_jumps           number of random jumps to add to the chain
 * @param jmp_old_chain       an existing jump chain; used with num_matching_jumps (size must be at least num_matching_jumps) (may be set to NULL)
 * @param num_matching_jumps  the number of jumps towards the end of the chain that should match jmp_old_chain
 * @param jmp_new_chain       a list of size at least num_jumps (may be set to NULL)
 *
 * @returns the location of the first jump (jump here to start the chain)
 */
u64 bhb_rand_nearjmp_chain_addr(u64 real_addr, u64 memory, size_t memory_size, size_t num_jumps, u64 *jmp_old_chain, size_t num_matching_jumps, u64 *jmp_new_chain);
