# SPDX-License-Identifier: GPL-3.0-only
import re
import json
import os


def load_out_dir(out_path):
    data_sets = {}
    for host_dir_name in os.listdir(out_path):
        if not host_dir_name.startswith("."):
            host_dir = os.path.join(out_path, host_dir_name)
            data_sets[host_dir_name] = {}
            data_sets[host_dir_name]["data"] = output_to_dict(
                os.path.join(host_dir, "run.out"))
            data_sets[host_dir_name]["metadata"] = metadata_to_dict(
                os.path.join(host_dir, "metadata.json"))
    return data_sets


def output_to_dict(file_path):
    result = {}

    with open(file_path) as f:
        start = False
        for line in f:
            if line.startswith("### RESULTS START ###"):
                start = True
                continue
            if line.startswith("### RESULTS END ###"):
                start = False
                continue
            if start:
                match = re.match(r"(?P<key>[^ ]+) = (?P<value>.+)", line)
                result[match.group("key")] = json.loads(match.group("value"))

    return result


def metadata_to_dict(file_path):
    with open(file_path) as f:
        return json.load(f)
