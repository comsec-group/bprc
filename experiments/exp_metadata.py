# SPDX-License-Identifier: GPL-3.0-only

CORE_RAPTOR_COVE = {
    "march": "Raptor Cove (RPL)", "type": "P-Core"}
CORE_RAPTOR_COVE_R = {"march": "Raptor Cove (RPL-R)", "type": "P-Core"}

CORE_ENHANCED_GRACEMONT_R = {"march": "Gracemont (RPL-R)", "type": "E-Core"}
CORE_ENHANCED_GRACEMONT = {"march": "Gracemont (RPL)", "type": "E-Core"}


CORE_GOLDEN_COVE_AL = {"march": "Golden Cove (ADL)", "type": "P-Core"}
CORE_GRACEMONT = {"march": "Gracemont (ADL)", "type": "E-Core"}

CORE_GOLDEN_COVE_SR = {"march": "Golden Cove (SPR)", "type": "P-Core"}

CORE_CYPRESS_COVE = {"march": "Cypress Cove (RKL)"}

CORE_COMET_LAKE = {"march": "Skylake (CML)"}

CORE_COFFEE_LAKE_R = {"march": "Skylake (CFL-R)"}

CORE_ZEN_4 = {"march": "Zen 4"}

CORE_ZEN_5 = {"march": "Zen 5"}

CORE_A55 = {"march": "Cortex-A55"}
CORE_A76 = {"march": "Cortex-A76"}
CORE_X1 = {"march": "Cortex-X1"}

def CORE_LOCAL(i):
    return {"march": f"Manual Run (local core {i})"}

SERVER_MAP = {
    "ee-tik-cn139": {
        "code_name": "Raptor Lake Refresh",
        "name": "Core i7-14700K",
        "microcode": "0x11d",
        "os": "Ubuntu 22.04 LTS",
        "kernel_version": "6.5.0",
        "cores": [
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_RAPTOR_COVE_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
            CORE_ENHANCED_GRACEMONT_R,
        ],
    },
    "ee-tik-cn138": {
        "code_name": "Sapphire Rapids",
        "name": "Xeon Silver 4510",
        "microcode": "0x2b000590",
        "os": "Ubuntu 22.04 LTS",
        "kernel_version": "6.5.0",
        "cores": [
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
            CORE_GOLDEN_COVE_SR,
        ],
    },
    "ee-tik-cn122": {
        "code_name": "Raptor Lake",
        "name": "Core i7-13700K",
        "microcode": "0x129",
        "os": "Ubuntu 24.04 LTS",
        "kernel_version": "6.8.0-47-generic",
        "cores": [
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_RAPTOR_COVE,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
            CORE_ENHANCED_GRACEMONT,
        ],
    },
    "ee-tik-cn114": {
        "code_name": "Alder Lake",
        "name": "Core i7-12700K",
        "microcode": "0x035",
        "os": "Ubuntu 20.04 LTS",
        "kernel_version": "5.15.0",
        "cores": [
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GOLDEN_COVE_AL,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
            CORE_GRACEMONT,
        ],
    },
    "ee-tik-cn109": {
        "code_name": "Rocket Lake",
        "name": "Core i7-11700K",
        "microcode": "0x59",
        "os": "Ubuntu 22.04 LTS",
        "kernel_version": "6.2.0",
        "cores": [
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
            CORE_CYPRESS_COVE,
        ],
    },
    "ee-tik-cn112": {
        "code_name": "Comet Lake",
        "name": "Core i7-10700K",
        "microcode": "0xf8",
        "os": "Ubuntu 22.04 LTS",
        "kernel_version": "5.15.0",
        "cores": [
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
            CORE_COMET_LAKE,
        ],
    },
    "ee-tik-cn103": {
        "code_name": "Coffee Lake Refresh",
        "name": "Core i9-9900K",
        "microcode": "0x100",
        "os": "Ubuntu 20.04 LTS",
        "kernel_version": "5.15.0",
        "cores": [
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
            CORE_COFFEE_LAKE_R,
        ],
    },
    "ee-tik-cn140": {
        "code_name": "Zen 5",
        "name": "Ryzen 9 9900X 12-Core Processor",
        "microcode": "0xb40401c",
        "os": "Ubuntu 22.04 LTS",
        "kernel_version": "6.11.3",
        "cores": [
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
            CORE_ZEN_5,
        ],
    },
    "ee-tik-cn126": {
        "code_name": "Zen 4",
        "name": "Ryzen 7 7700X",
        "microcode": "0xa601203",
        "os": "Ubuntu 20.04 LTS",
        "kernel_version": "5.15.0",
        "cores": [
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
        ],
    },
    "ee-tik-cn128": {
        "code_name": "Zen 4",
        "name": "Ryzen 7 7700X",
        "microcode": "0xa601203",
        "os": "Ubuntu 22.04 LTS",
        "kernel_version": "5.15.0",
        "cores": [
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
            CORE_ZEN_4,
        ],
    },
    "pixel-6": {
        "code_name": "Whitechapel",
        "name": "Google Tensor",
        "cores": [
            CORE_A55,
            CORE_A55,
            CORE_A55,
            CORE_A55,
            CORE_A76,
            CORE_A76,
            CORE_X1,
            CORE_X1,
        ],
    },
    "manual": {
        "code_name": "Local Processor (manual run)",
        "cores": [CORE_LOCAL(i) for i in range(256)]
    },
}


def order_data_sets(data_sets):
    sorted_sets = []
    # sort according to server map
    for server in SERVER_MAP:
        selected_sets = []
        # look for the correct data sets
        for data_set_name in data_sets:
            data_set = data_sets[data_set_name]
            if data_set["metadata"]["hostname"] == server:
                selected_sets += [data_set]

        # sort the selected sets according to the core number
        selected_sets = sorted(
            selected_sets, key=lambda d: d["metadata"]["experiment_core"] if "experiment_core" in d["metadata"] else 0)
        sorted_sets += selected_sets
    return sorted_sets
