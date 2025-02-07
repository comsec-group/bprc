# SPDX-License-Identifier: GPL-3.0-only
import json
import os
import sys
from os import path

import matplotlib.pyplot as p
import matplotlib

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_metadata
import exp_format

def arr_add(arr, sub):
    return [f + sub for f in arr]


font = {'size': 10}
matplotlib.rc('font', **font)

test_diff = 0

jump_diff = -0.03 + test_diff
call_diff = 0.00 + test_diff
ret_diff = 0.03 + test_diff

vertical_margin = 0.1


def diff_to_string(diff):
    return f"{'+' if diff >= 0 else ''}{diff}"


def create_plot(data_sets):
    THICKNESS = 2
    ROWS = 3
    COLUMNS = 4
    HEIGHT = 4.5
    WIDTH = 2 * HEIGHT
    fig, ax = p.subplots(ROWS, COLUMNS, figsize=(
        WIDTH, HEIGHT), layout="constrained")

    for i, data_set in enumerate(data_sets):
        label = None
        axis = ax[i // COLUMNS][i % COLUMNS]
        axis.set_title(
            exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]["cores"][
                int(data_set["metadata"]["experiment_core"])
            ]["march"],
        )
        # tiny shift to be able to see both jump and call in graph
        y = arr_add(data_set["data"]
                    ["jump_avg_mispredictions"], jump_diff)[:-1]
        if i == 0:
            label = f"jump* ({diff_to_string(jump_diff)})"
        axis.plot(
            [x + jump_diff for x in range(len(y))],
            y,
            label=label,
            zorder=4,
            # linestyle='dotted',
            linewidth=THICKNESS,
        )
        # tiny shift to be able to see both jump and call in graph
        y = arr_add(data_set["data"]
                    ["call_avg_mispredictions"], call_diff)[:-1]
        if i == 0:
            label = f"call* ({diff_to_string(call_diff)})"
        axis.plot(
            [x + call_diff for x in range(len(y))],
            y,
            label=label,
            zorder=3,
            # linestyle='dashed'
            linewidth=THICKNESS,
        )
        if "ret_avg_mispredictions" in data_set["data"]:
            y = arr_add(data_set["data"]
                        ["ret_avg_mispredictions"], ret_diff)[:-1]
            if i == 0:
                label = f"ret ({diff_to_string(ret_diff)})"
            axis.plot(
                [x + ret_diff for x in range(len(y))],
                y,
                label=label,
                zorder=2,
                linewidth=THICKNESS,
            )

        axis.grid(linestyle="dashed")
        axis.set_xticks([0, 256, 512, 768, 1024])
        # axis.set_xticks([0, 512, 1024])
        if i // COLUMNS == ROWS - 1:
            axis.set_xlabel("# of NOP instructions")
        else:
            axis.set_xticklabels([])

        axis.set_ylim(1 - vertical_margin, 2 + vertical_margin)
        # axis.set_ylim(-1, 2.20)
        axis.set_yticks([1, 1.5, 2])
        if i % COLUMNS != 0:
            axis.set_yticklabels([])
        else:
            axis.set_ylabel("avg misp.")

    fig.legend(
        loc="outside lower center",
        ncols=3,
    )

    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "figures")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, "btb_delay.pdf")

    fig.savefig(output_file)


def main():
    data_dir = path.join(SCRIPT_DIR, "out")
    data_sets = exp_format.load_out_dir(data_dir)
    data_sets = exp_metadata.order_data_sets(data_sets)

    create_plot(data_sets)


if __name__ == "__main__":

    main()
