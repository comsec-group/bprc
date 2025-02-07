// SPDX-License-Identifier: GPL-3.0-only
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include "./memtools.h"

#define MMAP_FLAGS (MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED_NOREPLACE)
#define PROT_RW    (PROT_READ | PROT_WRITE)
#define PROT_RWX   (PROT_RW | PROT_EXEC)
#define PG_ROUND(n) (((((n)-1UL)>>12)+1)<<12)

static inline long _va_to_phys(int fd, unsigned long va) {
    unsigned long pa_with_flags;
    pread(fd, &pa_with_flags, 8, (va & ~0xffful) >> 9);
    return pa_with_flags << 12 | (va & 0xfff);
}

/* translate virtual to physical address. requires root. */
long va_to_phys(void *va) {
    static int fd_pagemap;
    if (!fd_pagemap) {
        fd_pagemap = open("/proc/self/pagemap", O_RDONLY);
    }
    return _va_to_phys(fd_pagemap, _ul(va));
}

int map_translate(struct j_malloc *m) {
    int unmapped = 0;
    int pgs = m->pgs;

    // only possible with getuid()==0
    m->phys = calloc(pgs, sizeof(u64));
    int pg_sz = m->map_sz / m->pgs;
    for (int i = 0; i < pgs; ++i) {
        char *adr = m->map_base + i*pg_sz;
        *adr = i; // ensure populated
        m->phys[i] = va_to_phys(adr);
        if (m->phys[i] == 0) {
            ++unmapped;
        }
    }

    return unmapped;
}

int _map_exec(struct j_malloc *m, u64 addr, u64 code_sz, int flags)
{
    m->ptr = (void *)addr;
    m->map_base = (void *)((u64)m->ptr & ~0xfff);
    m->map_sz = PG_ROUND(((u64)m->ptr & 0xfff) + code_sz);
    m->code_sz = code_sz;
    if (mmap(m->map_base, m->map_sz, PROT_RWX, flags, -1, 0) ==
        MAP_FAILED) {
        m->map_base = 0;
        return -1;
    }

    if (flags & MAP_HUGETLB) {
        m->pgs = m->map_sz >> 21;
    } else {
        m->pgs = m->map_sz >> 12;
    }

    /* we can attach an array of phys pages the mapping uses
     * through map_translate. */
    m->phys = 0;

    return 0;
}

int map_exec(struct j_malloc *m, u64 addr, u64 code_sz)
{
    return _map_exec(m,addr,code_sz, MMAP_FLAGS);
}

int map_code(struct j_malloc *m, u64 addr, void *code_tmpl, u64 code_sz)
{
    if (map_exec(m,addr,code_sz)) {
        return -1;
    }

    memcpy(m->ptr, code_tmpl, code_sz);
    m->tmpl = code_tmpl;
    return 0;
}

int map_code_rand(struct j_malloc *m, void *code_tmpl, u64 code_sz, u64 mask)
{
    for (int try = 200; try--;) {
        if (map_code(m, rand47() & mask, code_tmpl, code_sz) == -1) {
            continue;
        }
        return 0;
    }
    return -1;
}

int junmap(struct j_malloc *m)
{
    int res = 0;
    if (m->map_base != 0) {
        // if no pointer, assume nothing was ever mapped
        res = munmap(m->map_base, m->map_sz);
    }
    if (m->phys != 0) {
        free(m->phys);
    }
    memset(m, 0, sizeof(*m));
    return res;
}

void code_poke(void *addr, char *code, int len)
{
    int result;
    void *addr_pg = (void *)((long)addr & ~0xfffUL);
    result = mprotect(addr_pg, len, PROT_RWX);
    if (result != 0) {
        err(result, "set rwx");
    }
    memcpy(addr, code, len);
    result |= mprotect(addr_pg, len, PROT_READ | PROT_EXEC);
    if (result != 0) {
        err(result, "set rx");
    }
}
