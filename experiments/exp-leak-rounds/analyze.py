# SPDX-License-Identifier: GPL-3.0-only
import json
import os
import sys
from os import path

import matplotlib.pyplot as p

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_format
import exp_metadata

MAX_POINTS = 16


def create_plot(data_sets):
    fig, ax = p.subplots(1, 2, figsize=(5, 2.5), layout="constrained")

    key = "jump_results_btb_attempts"
    for data_set in data_sets:
        y = [f / 1000 for f in data_set["data"][key][:MAX_POINTS]]
        x = range(1, len(y) + 1)
        ax[0].plot(
            x,
            y,
            label=exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]["cores"][
                int(data_set["metadata"]["experiment_core"])
            ]["march"],
        )

        y = [f / 1000 for f in data_set["data"][f"dis_{key}"][:MAX_POINTS]]
        x = range(1, len(y) + 1)
        ax[1].plot(
            x,
            y,
        )

    for i in range(2):
        ax[i].grid()
        ax[i].set_xlabel("repetition")
        # ax[i].set_xticks(range(1, MAX_POINTS + 1, 1))
        ax[i].set_ylim(-5, 105)
        yticks = [0, 25, 50, 75, 100]
        ax[i].set_yticks(yticks)
        ax[i].set_yticklabels([f"{t}%" for t in yticks])

    ax[0].set_ylabel("success rate")
    ax[0].set_title("eIBRS")
    ax[1].set_title("eIBRS + BHI_DIS_S")
    ax[1].set_yticklabels([])

    fig.legend(
        loc="outside lower center",
        ncols=3,
    )
    # fig.suptitle("BPI Primitive by Repetition")

    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "figures")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, "leak_rounds.pdf")
    fig.savefig(output_file)


def main():

    data_dir = path.join(SCRIPT_DIR, "out")
    data_sets = exp_format.load_out_dir(data_dir)
    data_sets = exp_metadata.order_data_sets(data_sets)

    create_plot(data_sets)


if __name__ == "__main__":
    main()
