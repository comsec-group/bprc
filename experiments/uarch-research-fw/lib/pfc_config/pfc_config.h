// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../compiler.h"

/** pmux config format as defined in the linux kernel */
union x86_pmu_config
{
    struct
    {
        u64 event : 8,
            umask : 8,
            usr : 1,
            os : 1,
            edge : 1,
            pc : 1,
            interrupt : 1,
            __reserved1 : 1,
            en : 1,
            inv : 1,
            cmask : 8,
            event2 : 4,
            __reserved2 : 4,
            go : 1,
            ho : 1;
    } bits;
    u64 value;
};
#define X86_CONFIG(...) ((union x86_pmu_config){.bits = {__VA_ARGS__}}).value
