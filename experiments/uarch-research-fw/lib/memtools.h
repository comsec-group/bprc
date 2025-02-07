// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <stdlib.h>
#include "compiler.h"
long va_to_phys(void *va);

static inline u64 rand47(void) {
	return (((unsigned long)random())<<16) ^ random();
}

static inline u64 rand38(void) {
       return (((unsigned long)random())<<7) ^ random();
}

// memory mapping
struct j_malloc {
    union {
        unsigned char *ptr;
        u64 ptr_u64;
        u64 (*fptr)(void);
    };
    // start page
    void *map_base;
    int pgs; // # pages
    u64 map_sz;
    u64 code_sz;
    u64* phys;
    void *tmpl; // often we build allocations from a template snip of code
    // map_base           map_base+map_sz
    // |----------------------|
    //        <-------->
    //        ptr     ptr+code_sz
};

/* legacy: Let's call them code_snip_t instead of struct j_malloc */
typedef struct j_malloc code_snip_t;

/**
 * makes an RWX mapping.
 * returns 0 on success, -1 otherwise
 */
int map_exec(struct j_malloc *m, u64 addr, u64 code_sz);

/**
 * maps code with map_exec
 * returns 0 on success, -1 otherwise
 */
int map_code(struct j_malloc *m, u64 addr, void *code_tmpl, u64 code_sz);

/**
 * @param u64 mask To give the code a certain alignment. Use mask = -1 for any
 * alignment, ~0xfUL for 16 byte alignment, etc,
 * returns 0 on success, -1 otherwise
 */
int map_code_rand(struct j_malloc *m, void *code_tmpl, u64 code_sz, u64 mask);

/**
 * returns 0 on success, -1 otherwise
 */
int junmap(struct j_malloc *m);

/**
 * Make mapping at addr RWX and write code to addr.
 */
void code_poke(void *addr, char *code, int len);


/**
 * Resolves the physical address of all pages.
 * uses calloc..
 */
int map_resolve_paddr(struct j_malloc *);

// ----------------------------------------------------------------------------
// - code templating ----------------------------------------------------------
// ----------------------------------------------------------------------------
asm(".section .text2, \"ax\"");

// labels right next to each other are made into one. Compilers remove labels
// if they reside at the same address, so make sure the `e_` label is kept by
// putting a nop after it.
#define mk_snip(name, str)\
    extern char name##__snip[]; \
    extern char e_##name##__snip[]; \
    asm(".pushsection .text2     \n"\
        ".balign 0x1000           \n"\
        #name"__snip:            \n"\
        str                         \
        "\ne_"#name"__snip: nop  \n"\
        ".popsection             \n"\
        )\

#define snip_sz(name) (unsigned long) (e_##name##__snip - name##__snip)

/* Allocate snip `snip` at `addr`. */
#define init_snip(desc, addr, snip) map_code(desc, addr, snip##__snip, snip_sz(snip))

/* Same as above, but randomize bits set in `mask` */
#define init_snip_rand(desc, mask, snip) map_code_rand(desc, snip##__snip, snip_sz(snip), mask)

#define snipcpy(dst, snip) memcpy(dst, snip##__snip, snip_sz(snip))

