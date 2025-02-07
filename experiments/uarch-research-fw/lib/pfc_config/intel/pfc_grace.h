// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include "../pfc_config.h"

#define ECORE_QUIRK 1 // for some reason we need to use a weird undocumented pfc config on the efficiency cores.

// config values for Alder/Raptor Lake (Refresh) E-Core

/** Counts the number of mispredicted near indirect JMP and near indirect CALL branch instructions retired. CORE: ECore */
#define BR_MISP_RETIRED__INDIRECT X86_CONFIG(.event = 0xC5, .umask = 0xEB)
/** Counts the number of mispredicted near RET branch instructions retired. CORE: ECore */
#define BR_MISP_RETIRED__RET X86_CONFIG(.event = 0xC5, .umask = 0xF7)
/** Counts the number of mispredicted near indirect CALL branch instructions retired. CORE: ECore */
#define BR_MISP_RETIRED__INDIRECT_CALL X86_CONFIG(.event = 0xC5, .umask = 0xFB)
