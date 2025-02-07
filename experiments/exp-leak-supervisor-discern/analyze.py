# SPDX-License-Identifier: GPL-3.0-only
import os
import sys
from os import path

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_format
import exp_metadata

def get_success_pct(hits_per_round):
    pct = round((hits_per_round / 100000) * 100, ndigits=1)

    color = 0
    if pct > 0:
        color = 20 + round(pct * 0.6)
    return f"{pct}%"


def main():
    data_dir = path.join(SCRIPT_DIR, "out")

    data_sets = exp_format.load_out_dir(data_dir)

    metric = "count_gt0"

    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "tables")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, "eibrs-leak.tex")
    data_sets = exp_metadata.order_data_sets(data_sets)

    for data_set in data_sets:
        server_map = exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]
        print("Server:", server_map["cores"]
              [int(data_set["metadata"]["experiment_core"])]["march"])
        jmp = get_success_pct(
            data_set["data"][f"jump_hits_per_round_btb_{metric}"])
        call = get_success_pct(
            data_set["data"][f"call_hits_per_round_btb_{metric}"])
        ret = get_success_pct(
            data_set["data"][f"ret_hits_per_round_btb_{metric}"])
        check = get_success_pct(max(data_set["data"][f"jump_hits_per_round_check_count_gt0"],
                                    data_set["data"][f"call_hits_per_round_check_count_gt0"],
                                    data_set["data"][f"ret_hits_per_round_check_count_gt0"]))
        print(f"BTB (jmp, call, ret, noise): {jmp}, {call}, {ret}, {check}")
        jmp = get_success_pct(
            data_set["data"][f"jump_hits_per_round_ibp_{metric}"])
        call = get_success_pct(
            data_set["data"][f"call_hits_per_round_ibp_{metric}"])
        ret = get_success_pct(
            data_set["data"][f"ret_hits_per_round_ibp_{metric}"])
        ret = get_success_pct(max(data_set["data"][f"jump_hits_per_round_check_count_gt0"],
                                  data_set["data"][f"call_hits_per_round_check_count_gt0"],
                                  data_set["data"][f"ret_hits_per_round_check_count_gt0"]))
        print(f"IBP (jmp, call, ret, noise): {jmp}, {call}, {ret}, {check}")


if __name__ == "__main__":

    main()
