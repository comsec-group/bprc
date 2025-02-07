# SPDX-License-Identifier: GPL-3.0-only
import json
import os
import sys
from os import path

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))

import exp_metadata
import exp_format

PREFIX = r"""
\newcommand{\doNohit}{\cellcolor{tabred!50}}
\newcommand{\doHit}{\cellcolor{tabgreen!20}}

\begin{tabular}{lrrrrrr}
    \hline
    \multirow{2}{*}{\textbf{Microarch.}} & \multicolumn{3}{c}{\textbf{ No Mitigation}} & \multicolumn{3}{c}{\textbf{BHI\_DIS\_S}} \\
    & \textit{jump*} & \textit{call*} & \textit{ret} & \textit{jump*} & \textit{call*} & \textit{ret} \\
    \hline
"""
POSTFIX = r"""    \hline
\end{tabular}
"""

def get_mark(misp):
    # return r"\xmark" if misp > 0 else r"\cmark"
    misp *= 100
    val = min(100, round(misp))

    if (val >= 99):
        return f"{val}\\% \\doNohit"
    else:
        return f"{val}\\% \\doHit"

def main():
    eval_func = "avg"

    data_dir = path.join(SCRIPT_DIR, "out")

    data_sets = exp_format.load_out_dir(data_dir)

    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "tables")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, "bhi-dis-s.tex")
    data_sets = exp_metadata.order_data_sets(data_sets)
    with open(output_file, "w") as f:
        f.write(PREFIX)
        for i, data_set in enumerate(data_sets):
            server_metadata = exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]
            core_name = server_metadata["cores"][data_set["metadata"]["experiment_core"]]["march"]
            f.write("    ")
            f.write(core_name)
            if i % 2 == 0:
                f.write(" \\colorLightGrey")
            f.write(" & ")
            f.write(
                get_mark(
                    data_set["data"][f"jump_{eval_func}"]
                    - data_set["data"][f"jump_{eval_func}_call"]
                )
            )
            f.write(" & ")
            f.write(get_mark(data_set["data"][f"call_{eval_func}"]))
            f.write(" & ")
            # gracemont has no RSBA
            if "Gracemont" in core_name:
                f.write(r"\multicolumn{1}{c}{-}")
            else:
                f.write(get_mark(data_set["data"][f"ret_{eval_func}"]))
            f.write(" & ")
            f.write(
                get_mark(
                    data_set["data"][f"dis_jump_{eval_func}"]
                    - data_set["data"][f"dis_jump_{eval_func}_call"]
                )
            )
            f.write(" & ")
            f.write(get_mark(data_set["data"][f"dis_call_{eval_func}"]))
            f.write(" & ")
            # gracemont has no RSBA
            if "Gracemont" in core_name:
                f.write(r"\multicolumn{1}{c}{-}")
            else:
                f.write(get_mark(data_set["data"][f"dis_ret_{eval_func}"] - 1))
            f.write(r" \\")
            f.write("\n")

        f.write(POSTFIX)


if __name__ == "__main__":

    main()
