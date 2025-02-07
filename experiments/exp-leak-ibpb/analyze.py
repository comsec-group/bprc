# SPDX-License-Identifier: GPL-3.0-only
import os
import sys
from os import path

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_format
import exp_metadata

METRIC = "count_gt0"
TOTAL_ITERATIONS = 100000

def _get_pct(hits_per_round):
    return round((hits_per_round / TOTAL_ITERATIONS) * 100, ndigits=1)


def main():
    data_dir = path.join(SCRIPT_DIR, "out")
    data_sets = exp_format.load_out_dir(data_dir)
    data_sets = exp_metadata.order_data_sets(data_sets)

    for i, data_set in enumerate(data_sets):
        server_map = exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]
        print(server_map["cores"][data_set["metadata"]["experiment_core"]]["march"])
        print("  "
            f'jump: {_get_pct(data_set["data"][f"jump_hits_per_round_{METRIC}"])}%', 
            f'call: {_get_pct(data_set["data"][f"call_hits_per_round_{METRIC}"])}%, '
            f'ret: {_get_pct(data_set["data"][f"ret_hits_per_round_{METRIC}"])}%, '
            f'check: {_get_pct(max(data_set["data"][f"jump_check"],data_set["data"][f"call_check"],data_set["data"][f"ret_check"]))}%'
        )


if __name__ == "__main__":
    main()
