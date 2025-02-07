// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../pfc_config.h"

/**
 * [Retired Indirect Branch Instructions Mispredicted] (Core::X86::Pmc::Core::ExRetBrnIndMisp)
 * The number of indirect branches retired that were not correctly predicted. Each such mispredict incurs the same penalty 
 * as a mispredicted conditional branch instruction. Note that only EX mispredicts are counted.
 */
#define BR_MISP_RETIRED__INDIRECT X86_CONFIG(.event = 0xCA, .umask = 0x00)

/**
 * [Retired Near Returns Mispredicted] (Core::X86::Pmc::Core::ExRetNearRetMispred)
 * The number of near returns retired that were not correctly predicted by the return address predictor. Each such mispredict
 * incurs the same penalty as a mispredicted conditional branch instruction.
 */
#define BR_MISP_RETIRED__RET X86_CONFIG(.event = 0xC9, .umask = 0x00)
