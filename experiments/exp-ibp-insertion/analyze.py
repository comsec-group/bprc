# SPDX-License-Identifier: GPL-3.0-only
import os
import sys
from os import path

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_format
import exp_metadata

METRIC = "count_gt0"


def _get_pct(hits_per_round):
    pct_str = f"{round((hits_per_round / 100000) * 100, ndigits=1)}"

    while len(pct_str) < 4:
        pct_str = f" {pct_str}"

    return pct_str



def main():
    data_dir = path.join(SCRIPT_DIR, "out")
    data_sets = exp_format.load_out_dir(data_dir)
    data_sets = exp_metadata.order_data_sets(data_sets)

    for i, data_set in enumerate(data_sets):
        server_map = exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]
        print(server_map["cores"][data_set["metadata"]
              ["experiment_core"]]["march"])
        print("  Random History:  ")
        print("    inst.| BTB   | IBP   |")
        print(f'    jump | {_get_pct(data_set["data"][f"rand_jump_hits_per_round_btb_{METRIC}"])}% | {_get_pct(data_set["data"][f"rand_jump_hits_per_round_ibp_{METRIC}"])}% |')
        print(f'    call | {_get_pct(data_set["data"][f"rand_call_hits_per_round_btb_{METRIC}"])}% | {_get_pct(data_set["data"][f"rand_call_hits_per_round_ibp_{METRIC}"])}% |')
        print(f'    ret  | {_get_pct(data_set["data"][f"rand_ret_hits_per_round_btb_{METRIC}"])}% | {_get_pct(data_set["data"][f"rand_ret_hits_per_round_ibp_{METRIC}"])}% |')

        print("  Matching History:  ")
        print("    inst.| BTB   | IBP   |")
        print(f'    jump | {_get_pct(data_set["data"][f"match_jump_hits_per_round_btb_{METRIC}"])}% | {_get_pct(data_set["data"][f"match_jump_hits_per_round_ibp_{METRIC}"])}% |')
        print(f'    call | {_get_pct(data_set["data"][f"match_call_hits_per_round_btb_{METRIC}"])}% | {_get_pct(data_set["data"][f"match_call_hits_per_round_ibp_{METRIC}"])}% |')
        print(f'    ret  | {_get_pct(data_set["data"][f"match_ret_hits_per_round_btb_{METRIC}"])}% | {_get_pct(data_set["data"][f"match_ret_hits_per_round_ibp_{METRIC}"])}% |')


if __name__ == "__main__":
    main()
