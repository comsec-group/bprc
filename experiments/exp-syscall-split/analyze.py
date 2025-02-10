# SPDX-License-Identifier: GPL-3.0-only
import json
import os
import sys
from os import path

import matplotlib.pyplot as p
import matplotlib
matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42

SCRIPT_DIR = path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_format
import exp_metadata

NUM_ROUNDS = 100000

def create_plot(data_sets):
    fig, ax = p.subplots(1, 1, figsize=(5, 2.5), layout="constrained")

    for i, data_set in enumerate(data_sets):

        server_map = exp_metadata.SERVER_MAP[data_set["metadata"]["hostname"]]
        # use colors that match the graphic
        for key, k_label, color in [
            ("jump_result_btb_wait_supervisor", "kernel", "#DB7A7A"),
            ("jump_result_btb_wait_user", "user", "#8CBF88"),
        ]:
            label = None
            if i == 0:
                label = k_label
            y = [f / (NUM_ROUNDS / 100) for f in data_set["data"][key]]
            ax.plot(range(len(y)), y, label=label, color=color)

        ax.set_title(server_map["cores"][int(
            data_set["metadata"]["experiment_core"])]["march"])

        ax.grid(linestyle="dashed")
        ax.set_xlabel("# of NOP instructions delay")
        ax.set_xticks([x for x in range(0, 513, 64)])
        ax.set_ylim(-5, 105)
        ax.set_yticks([0, 25, 50, 75, 100])

        ax.set_ylabel("gadget hit rate")

        ax.set_yticklabels(["0%", "25%", "50%", "75%", "100%"])

        fig.legend(
            loc="outside lower center",
            ncols=2,
        )

    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "figures")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, f"syscall_split.pdf")
    fig.savefig(output_file)



def main():
    data_dir = path.join(SCRIPT_DIR, "out")
    data_sets = exp_format.load_out_dir(data_dir)
    data_sets = exp_metadata.order_data_sets(data_sets)

    create_plot(data_sets)


if __name__ == "__main__":
    main()
