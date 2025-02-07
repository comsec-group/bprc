// SPDX-License-Identifier: GPL-3.0-only
// PMU configuration
#define ARM_PMU_FILTER_EL1_MASK (1ul << 31) // filter all EL1 (kernel) events to reduce noise
#define ARM_PMU_FILTER_EL2_MASK (1ul << 27) // filter all EL2 events to reduce noise
#define ARM_PMU_FILTER_EL3_MASK (1ul << 26) // filter all EL2 events to reduce noise

/** Instruction architecturally executed. This event counts all retired instructions, including those that fail their condition check. */
#define INST_RETIRED 0x8
/** Mispredicted or not predicted branch speculatively executed. This event counts any predictable branch instruction which is mispredicted either due to dynamic misprediction or because the MMU is off and the branches are statically predicted not taken. */
#define BR_MIS_PRED 0x10
/** Predictable branch speculatively executed. This event counts all predictable branches. */
#define BR_PRED 0x12
/** Instruction architecturally executed, branch. This event counts all branches, taken or not. This excludes exception entries, debug entries and CCFAIL branches. */
#define BR_RETIRED 0x21
/** Instruction architecturally executed, mispredicted branch. This event counts any branch counted by BR_RETIRED which is not correctly predicted and causes a pipeline flush. */
#define BR_MIS_PRED_RETIRED 0x22
