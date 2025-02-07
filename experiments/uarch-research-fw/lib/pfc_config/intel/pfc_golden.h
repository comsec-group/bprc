// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../pfc_config.h"

// config values for Alder/Raptor Lake (Refresh) P-Core

/** Number of times the front-end is resteered when it finds a branch instruction in a fetch line. This is called Unknown Branch which occurs for the first time a branch instruction is fetched or when the branch is not tracked by the BPU (Branch Prediction Unit) anymore. CORE: PCore EventSel=60H UMask=01H  */
#define BACLEARS__ANY X86_CONFIG(.event = 0x60, .umask = 0x01)

/** Counts all branch instructions retired. CORE: PCore EventSel=C4H UMask=00H  */
#define BR_INST_RETIRED__ALL_BRANCHES X86_CONFIG(.event = 0xC4)
/** Counts all the retired branch instructions that were mispredicted by the processor. A branch misprediction occurs when the processor incorrectly predicts the destination of the branch. When the misprediction is discovered at execution, all the instructions executed in the wrong (speculative) path must be discarded, and the processor must start fetching from the correct path. CORE: PCore EventSel=C5H UMask=00H   */
#define BR_MISP_RETIRED__ALL_BRANCHES X86_CONFIG(.event = 0xC5)
/** Counts conditional branch instructions retired. CORE: PCore EventSel=C4H UMask=11H  */
#define BR_INST_RETIRED__COND X86_CONFIG(.event = 0xC4, .umask = 0x11)
/** Counts mispredicted conditional branch instructions retired. CORE: PCore EventSel=C5H UMask=11H  */
#define BR_MISP_RETIRED__COND X86_CONFIG(.event = 0xC5, .umask = 0x11)
/** Counts near indirect branch instructions retired excluding returns. TSX abort is an indirect branch. CORE: PCore EventSel=C4H UMask=80H  */
#define BR_INST_RETIRED__INDIRECT X86_CONFIG(.event = 0xC4, .umask = 0x80)
/** Counts miss-predicted near indirect branch instructions retired excluding returns. TSX abort is an indirect branch. CORE: PCore EventSel=C5H UMask=80H */
#define BR_MISP_RETIRED__INDIRECT X86_CONFIG(.event = 0xC5, .umask = 0x80)
/** This is a non-precise version (that is, does not use PEBS) of the event that counts mispredicted return instructions retired. CORE: PCore */
#define BR_MISP_RETIRED__RET X86_CONFIG(.event = 0xC5, .umask = 0x08)
/** Counts retired mispredicted indirect (near taken) CALL instructions, including both register and memory indirect. CORE: PCore */
#define BR_MISP_RETIRED__INDIRECT_CALL X86_CONFIG(.event = 0xC5, .umask = 0x02)

/** Counts the number of PREFETCHNTA instructions executed. CORE: PCore */
#define SW_PREFETCH_ACCESS__NTA X86_CONFIG(.event = 0x40, .umask = 0x01)
/** Counts the number of PREFETCHW instructions executed. CORE: PCore */
#define SW_PREFETCH_ACCESS__PREFETCHW X86_CONFIG(.event = 0x40, .umask = 0x08)
/** Counts the number of PREFETCHT0 instructions executed. CORE: PCore */
#define SW_PREFETCH_ACCESS__T0 X86_CONFIG(.event = 0x40, .umask = 0x02)
/** Counts the number of PREFETCHT1 or PREFETCHT2 instructions executed. CORE: PCore */
#define SW_PREFETCH_ACCESS__T1_T2 X86_CONFIG(.event = 0x40, .umask = 0x04)

/** Counts retired load instructions with at least one uop that missed in the L1 cache. CORE: PCore */
#define MEM_LOAD_RETIRED__L1_MISS X86_CONFIG(.event = 0xD1, .umask = 0x08)
/** Counts retired load instructions missed L2 cache as data sources. CORE: PCore */
#define MEM_LOAD_RETIRED__L2_MISS X86_CONFIG(.event = 0xD1, .umask = 0x10)
/** Counts retired load instructions with at least one uop that missed in the L3 cache. CORE: PCore */
#define MEM_LOAD_RETIRED__L3_MISS X86_CONFIG(.event = 0xD1, .umask = 0x20)

/** Counts the number of cycles uops were delivered to Instruction Decode Queue (IDQ) from the Decode Stream Buffer (DSB) path. CORE: PCore */
#define IDQ__DSB_CYCLES_ANY X86_CONFIG(.event = 0x79, .umask = 0x08, .cmask = 0x01, )
/** Counts the number of cycles where optimal number of uops was delivered to the Instruction Decode Queue (IDQ) from the DSB (Decode Stream Buffer) path. Count includes uops that may 'bypass' the IDQ. CORE: PCore */
#define IDQ__DSB_CYCLES_OK X86_CONFIG(.event = 0x79, .umask = 0x08, .cmask = 0x06, )
/** Counts the number of uops delivered to Instruction Decode Queue (IDQ) from the Decode Stream Buffer (DSB) path. CORE: PCore */
#define IDQ__DSB_UOPS X86_CONFIG(.event = 0x79, .umask = 0x08, )

/** Counts the number of cycles uops were delivered to the Instruction Decode Queue (IDQ) from the MITE (legacy decode pipeline) path. During these cycles uops are not being delivered from the Decode Stream Buffer (DSB). CORE: PCore */
#define IDQ__MITE_CYCLES_ANY X86_CONFIG(.event = 0x79, .umask = 0x04, .cmask = 0x01, )
/** Counts the number of cycles where optimal number of uops was delivered to the Instruction Decode Queue (IDQ) from the MITE (legacy decode pipeline) path. During these cycles uops are not being delivered from the Decode Stream Buffer (DSB). CORE: PCore */
#define IDQ__MITE_CYCLES_OK X86_CONFIG(.event = 0x79, .umask = 0x04, .cmask = 0x06, )
/** Counts the number of uops delivered to Instruction Decode Queue (IDQ) from the MITE path. This also means that uops are not being delivered from the Decode Stream Buffer (DSB). CORE: PCore */
#define IDQ__MITE_UOPS X86_CONFIG(.event = 0x79, .umask = 0x04, )

/** Counts the number of uops not delivered to by the Instruction Decode Queue (IDQ) to the back-end of the pipeline when there was no back-end stalls. This event counts for one SMT thread in a given cycle. CORE: PCore */
#define IDQ_UOPS_NOT_DELIVERED__CORE X86_CONFIG(.event = 0x9C, .umask = 0x01, )
/** Counts the number of cycles when no uops were delivered by the Instruction Decode Queue (IDQ) to the back-end of the pipeline when there was no back-end stalls. This event counts for one SMT thread in a given cycle. CORE: PCore */
#define IDQ_UOPS_NOT_DELIVERED__CYCLES_0_UOPS_DELIV__CORE X86_CONFIG(.event = 0x9C, .umask = 0x01, .cmask = 0x06, )
/** Counts the number of cycles when the optimal number of uops were delivered by the Instruction Decode Queue (IDQ) to the back-end of the pipeline when there was no back-end stalls. This event counts for one SMT thread in a given cycle. CORE: PCore */
#define IDQ_UOPS_NOT_DELIVERED__CYCLES_FE_WAS_OK X86_CONFIG(.event = 0x9C, .umask = 0x01, .inv = 1, .cmask = 0x01, )

/** Counts the number of uops that the Resource Allocation Table (RAT) issues to the Reservation Station (RS). CORE: PCore */
#define UOPS_ISSUED__ANY X86_CONFIG(.event = 0xAE, .umask = 0x01, )
/** Counts cycles during which no uops were dispatched from the Reservation Station (RS) per thread. CORE: PCore */
#define UOPS_EXECUTED__STALLS X86_CONFIG(.event = 0xB1, .umask = 0x01, .inv = 1, .cmask = 0x01, )
/** Counts the number of uops to be executed per-thread each cycle. CORE: PCore */
#define UOPS_EXECUTED__THREAD X86_CONFIG(.event = 0xB1, .umask = 0x01, )
/** Counts the number of x87 uops executed. CORE: PCore */
#define UOPS_EXECUTED__X87 X86_CONFIG(.event = 0xB1, .umask = 0x10, )
/** This event counts cycles without actually retired uops. CORE: PCore */
#define UOPS_RETIRED__STALLS X86_CONFIG(.event = 0xC2, .umask = 0x02, .inv = 1, .cmask = 0x01, )
