# SPDX-License-Identifier: GPL-3.0-only
import re
import os
import statistics
import sys

SCRIPT_DIR = os.path.dirname(__file__)
sys.path.append(os.path.join(SCRIPT_DIR, ".."))
import exp_metadata

DATA_DIR = os.path.join(SCRIPT_DIR, "out")
UNIXBENCH_REGEX = r".+(\d+\.\d)\s+(?P<result>\d+\.\d)\s+(\d+\.\d)"
LMBENCH_REGEX = r".+: (?P<result>\d+\.\d+) .+"

PREFIX = r"""\begin{tabular}{lcc}
    \hline
    \multirow{2}{*}{\textbf{CPU}} & \multicolumn{2}{c}{\textbf{Mitigation}}                          \\
                                       & \textit{IPRED\_DIS\_S}                  & \textit{Retpoline}     \\
    \hline
"""
POSTFIX = r"""    \hline
\end{tabular}
"""


def load_unixbench_run(file_name):
    results = []
    with open(file_name) as f:
        for line in f:
            if "running 1 parallel" in line:
                break
        for line in f:
            if "parallel copies" in line:
                break

        data = f.readlines()[15:27]

        for l in data:
            match = re.match(UNIXBENCH_REGEX, l)
            results.append(int(match.group("result").replace(".", "")))

    return results


LMBENCH_METRICS = [
    "Simple syscall",
    "Simple read",
    "Simple write",
    "Simple stat",
    "Simple fstat",
    "Simple open/close",
    "Select on 10 fd's",
    "Signal handler installation",
    "Signal handler overhead",
    "Protection fault",
    "Pipe latency",
    "AF_UNIX sock stream latency",
    "Process fork+exit",
    "Process fork+execve",
    "Process fork+/bin/sh -c",
    "Pagefaults on /var/tmp/XXX",
    "UDP latency using localhost",
    "TCP latency using localhost",
    "TCP/IP connection cost to localhost",
]


def load_lmbench_run(file_name):
    results = []
    with open(file_name) as f:
        for l in f:
            for metric in LMBENCH_METRICS:
                if metric in l:
                    match = re.match(LMBENCH_REGEX, l)
                    data_point = int(match.group("result").replace(".", ""))
                    results.append(data_point)
                    break

    return results


def load_data_sets():
    data_sets = {}
    for host_name in os.listdir(DATA_DIR):
        if host_name.startswith("."):
            continue

        host_dir = os.path.join(DATA_DIR, host_name)
        host_data = {}
        host_data["metadata"] = {}
        host_data["metadata"]["hostname"] = host_name

        for configuration_name in os.listdir(host_dir):
            configuration_dir = os.path.join(host_dir, configuration_name)
            host_data[configuration_name] = {}

            unixbench_dir = os.path.join(configuration_dir, "unixbench")
            unixbench_data = []
            for result_name in os.listdir(unixbench_dir):
                if ".log" in result_name or ".html" in result_name:
                    continue

                result_file = os.path.join(unixbench_dir, result_name)
                data = load_unixbench_run(result_file)
                unixbench_data.append(data)
            host_data[configuration_name]["unixbench"] = unixbench_data

            lmbench_dir = os.path.join(configuration_dir, "lmbench")
            lmbench_data = []
            for result_name in os.listdir(lmbench_dir):
                result_file = os.path.join(lmbench_dir, result_name)
                data = load_lmbench_run(result_file)
                lmbench_data.append(data)
            host_data[configuration_name]["lmbench"] = lmbench_data
        data_sets[host_name] = host_data

    return data_sets


def calculate_mean(data_sets):
    new_data_sets = []
    for host_set in data_sets:
        new_host_set = {}
        for config in host_set:
            if config == "metadata":
                continue
            config_set = host_set[config]
            new_config_set = {}
            for benchmark in config_set:
                benchmark_data = config_set[benchmark]

                median_data = []
                for i in range(len(benchmark_data[0])):
                    subset = [data[i] for data in benchmark_data]
                    median_data.append(statistics.median(subset))

                mean_data = statistics.geometric_mean(median_data)

                new_config_set[benchmark] = mean_data
            new_host_set[config] = new_config_set
        new_host_set["metadata"] = host_set["metadata"]
        new_data_sets.append(new_host_set)
    return new_data_sets


def evaluate(data_sets):
    new_data_sets = []
    for host_set in data_sets:
        new_host_set = {}

        for config in host_set:
            if config == "baseline" or config == "metadata":
                continue

            config_set = host_set[config]
            new_config_set = {}
            for benchmark in config_set:
                benchmark_data = config_set[benchmark]

                benchmark_data_baseline = host_set["baseline"][benchmark]

                if benchmark == "unixbench":
                    new_benchmark_data = round(
                        ((benchmark_data_baseline / benchmark_data) - 1) * 100, ndigits=1)
                else:
                    assert benchmark == "lmbench"
                    new_benchmark_data = round(
                        (benchmark_data / benchmark_data_baseline - 1) * 100, ndigits=1)

                new_config_set[benchmark] = new_benchmark_data
            new_host_set[config] = new_config_set
        new_host_set["metadata"] = host_set["metadata"]
        new_data_sets.append(new_host_set)
    return new_data_sets


def save_table(results):
    output_dir = os.path.join(SCRIPT_DIR, "..", "..", "tables")
    os.makedirs(output_dir, exist_ok=True)
    output_file = os.path.join(output_dir, "mitigations.tex")
    with open(output_file, "w") as f:
        f.write(PREFIX)
        for i, host_set in enumerate(results):
            server_map = exp_metadata.SERVER_MAP[host_set["metadata"]["hostname"]]
            f.write("    ")
            f.write(server_map["code_name"])
            if i % 2 == 0:
                f.write(" \\colorLightGrey")
            f.write(" & ")
            if "ipred" in host_set:
                f.write(
                    f"{host_set['ipred']['unixbench']}\\% / {host_set['ipred']['lmbench']}\\%")
            else:
                f.write("-")
            if i % 2 == 0:
                f.write(" \\colorLightGrey")
            f.write(" & ")
            if "retpoline" in host_set:
                f.write(
                    f"{host_set['retpoline']['unixbench']}\\% / {host_set['retpoline']['lmbench']}\\%")
            else:
                f.write("-")
            if i % 2 == 0:
                f.write(" \\colorLightGrey")
            f.write(" \\\\\n")

            if "microcode" in host_set:
                print(f"{server_map['code_name']} Microcode: {host_set['microcode']['unixbench']}% / {host_set['microcode']['lmbench']}%")
        f.write(POSTFIX)


def main():
    data_sets = load_data_sets()
    data_sets = exp_metadata.order_data_sets(data_sets)

    mean_sets = calculate_mean(data_sets)

    results = evaluate(mean_sets)

    save_table(results)


if __name__ == "__main__":
    main()
