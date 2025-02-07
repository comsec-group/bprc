// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "compiler.h"

#define MSR_IA32_SPEC_CTRL 0x00000048 /* Speculation Control */
#define IBRS_BIT 0
#define STIBP_BIT 1
#define SSBD_BIT 2
#define IPRED_DIS_U_BIT 3
#define IPRED_DIS_S_BIT 4
#define RRSBA_DIS_U_BIT 5
#define RRSBA_DIS_S_BIT 6
#define BHI_DIS_S_BIT 10
#define IBRS_MASK (1ul << IBRS_BIT)
#define STIBP_MASK (1ul << STIBP_BIT)
#define SSBD_MASK (1ul << SSBD_BIT)
#define IPRED_DIS_U_MASK (1ul << IPRED_DIS_U_BIT)
#define IPRED_DIS_S_MASK (1ul << IPRED_DIS_S_BIT)
#define RRSBA_DIS_U_MASK (1ul << RRSBA_DIS_U_BIT)
#define RRSBA_DIS_S_MASK (1ul << RRSBA_DIS_S_BIT)
#define BHI_DIS_S_MASK (1ul << BHI_DIS_S_BIT)

#define MSR_IA32_PRED_CMD 0x00000049 /* Prediction Command */
#define IBPB_BIT 0
#define IBPB_MASK (1ul << IBPB_BIT)

#define MSR_EFER 0xC0000080 /* AMD EFER */
#define AIBRSE_BIT 21
#define AIBRSE_MASK (1ul << AIBRSE_BIT)

typedef struct msr_params
{
    u32 msr_nr;
    u64 msr_value;
} msr_params_t;

void msr_init(void);

noinline void msr_rdmsr(msr_params_t *c);
noinline void msr_wrmsr(msr_params_t *c);
