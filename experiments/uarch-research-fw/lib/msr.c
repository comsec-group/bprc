// SPDX-License-Identifier: GPL-3.0-only
#include <unistd.h>
#include "msr.h"

void msr_init(void)
{
    // avoid pagefaults in the kernel
    msr_rdmsr(NULL);
    msr_wrmsr(NULL);
}

noinline void msr_rdmsr(msr_params_t *c)
{
    if (c)
    {
        msr_params_t *params = c;

        u32 eax = 0;
        u32 edx = 0;
        u32 ecx = params->msr_nr;

        // clang-format off
        asm volatile (
            "rdmsr\n"
            :"=a"(eax), "=d"(edx):"c"(ecx):
        );
        // clang-format on

        params->msr_value = ((u64)edx) << 32 | eax;
    }
}

noinline void msr_wrmsr(msr_params_t *c)
{
    if (c)
    {
        msr_params_t *params = c;

        u32 eax = params->msr_value & 0xFFFFFFFF;
        u32 edx = (params->msr_value >> 32) & 0xFFFFFFFF;
        u32 ecx = params->msr_nr;

        // clang-format off
        asm volatile (
            "wrmsr\n"
            ::"a"(eax), "c"(ecx), "d"(edx):
        );
        // clang-format on
    }
}