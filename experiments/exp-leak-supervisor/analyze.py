# SPDX-License-Identifier: GPL-3.0-only
import os
import sys
from os import path

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_metadata
import exp_format

PREFIX = r"""\begin{tabular}{lrrrr}
    \hline
    \textbf{Microarchitecture} & \multicolumn{1}{c}{\inst{jump*}} & \multicolumn{1}{c}{\inst{call*}} & \multicolumn{1}{c}{\inst{ret}} & \multicolumn{1}{c}{\inst{noise}} \\
    \hline
"""
POSTFIX = r"""    \hline
    \multicolumn{4}{l}{$^a$ indistinguishable from noise}
\end{tabular}
"""
NOISE_PREFIX = r"$^a$"


def _get_pct(hits_per_round):
    return round((hits_per_round / 100000) * 100, ndigits=1)


def get_success_pct_nocolor(hits_per_round):
    pct = _get_pct(hits_per_round)
    return f"{pct}\\%"


def get_success_pct(hits_per_round):
    pct = _get_pct(hits_per_round)
    color = 0
    if pct > 0:
        color = 20 + round(pct * 0.6)
    return f"{pct}\\% \\cellcolor{{tabred!{color}}}"


def print_denoised_result(f, value, noise):
    if value > noise:
        f.write(get_success_pct(value))
    else:
        if value > 0:
            f.write(NOISE_PREFIX)
        f.write(get_success_pct_nocolor(value))


def main():
    data_dir = path.join(SCRIPT_DIR, "out")

    data_sets = exp_format.load_out_dir(data_dir)

    metric = "count_gt0"

    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "tables")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, "eibrs-leak.tex")
    data_sets = exp_metadata.order_data_sets(data_sets)
    with open(output_file, "w") as f:
        f.write(PREFIX)
        for i, data_set in enumerate(data_sets):
            server_map = exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]

            print(
                "Server:",
                server_map["cores"][int(data_set["metadata"]["experiment_core"])][
                    "march"
                ],
            )
            jmp = _get_pct(data_set["data"][f"jump_hits_per_round_{metric}"])
            call = _get_pct(data_set["data"][f"call_hits_per_round_{metric}"])
            ret = _get_pct(data_set["data"][f"ret_hits_per_round_{metric}"])
            check = _get_pct(
                max(
                    data_set["data"][f"jump_check"],
                    data_set["data"][f"call_check"],
                    data_set["data"][f"ret_check"],
                )
            )
            print(f"   (jmp, call, ret, noise): {jmp}%, {call}%, {ret}%, {check}%")

            # in the paper we only show the intel systems in the table
            if (
                data_set["metadata"]["hostname"] == "ee-tik-cn128"
                or data_set["metadata"]["hostname"] == "ee-tik-cn140"
            ):
                continue

            jump_result = data_set["data"][f"jump_hits_per_round_{metric}"]
            call_result = data_set["data"][f"call_hits_per_round_{metric}"]
            ret_result = data_set["data"][f"ret_hits_per_round_{metric}"]
            noise_result = max(
                data_set["data"][f"jump_check"],
                data_set["data"][f"call_check"],
                data_set["data"][f"ret_check"],
            )

            f.write("    ")
            f.write(
                server_map["cores"][int(data_set["metadata"]["experiment_core"])][
                    "march"
                ]
            )
            if i % 2 == 0:
                f.write(" \\colorLightGrey")

            f.write(" & ")
            print_denoised_result(f, jump_result, noise_result)
            f.write(" & ")
            print_denoised_result(f, call_result, noise_result)
            f.write(" & ")
            print_denoised_result(f, ret_result, noise_result)
            f.write(" & ")
            f.write(get_success_pct_nocolor(noise_result))
            f.write(r" \\")
            f.write("\n")
        f.write(POSTFIX)


if __name__ == "__main__":
    main()
