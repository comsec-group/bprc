# SPDX-License-Identifier: GPL-3.0-only
import os
import re
import statistics

ROOT_HASH = "root:$6$HGU6y/YF5$rf5na/CbCxzKWRjrkPYDb1oN5ZI7TSxVs8Epz79q9jUyZnIqmmIGYd4lzOVa8I4yyGQ2SSjyd0op5mujCdX4Q.:19755:0:99999:7:::"
LEAK_SIZE = 1048576

SCRIPT_DIR_PATH = os.path.dirname(__file__)
DATA_DIR_PATH = os.path.join(SCRIPT_DIR_PATH, "out/")

KASLR_TIME_REGEX = r".*kaslr time: (?P<kaslr_time>\d+.\d+)s"

RB_TIME_REGEX = r".*rb_offset time:\s+(?P<time>\d+.\d+)s"
PM_TIME_REGEX = r".*physmap_offset time:\s+(?P<time>\d+.\d+)s"
SHADOW_FIND_REGEX = r".*shadow search time:\s+(?P<time>\d+.\d+)s"
SHADOW_LEAK_REGEX = r".*shadow leak time:\s+(?P<time>\d+.\d+)s"
BANDWIDTH_SECRET_SIZE_REGEX = r".*secret_size:\s+(?P<size>\d+)b"
BANDWIDTH_LEAK_REGEX = r".*secret_leak:\s+(?P<time>\d+.\d+)s"
BANDWIDTH_MISTAKES_REGEX = r".*mistakes:\s+(?P<count>\d+)"


def s_to_ms(s):
    if s is None:
        return None
    return round(float(s) * 1000)


def collect_data(path, server):
    data_set = []
    server_path = os.path.join(path, server)
    for run_name in os.listdir(server_path):
        run_data = {}
        run_dir = os.path.join(server_path, run_name)

        kaslr_file_path = os.path.join(run_dir, f"{server}-kaslr.out")
        shadow_file_path = os.path.join(run_dir, f"{server}-shadow.out")
        bandwidth_file_path = os.path.join(run_dir, f"{server}-benchmark.out")

        # parse KASLR results
        # we assume KASLR was successful if the subsequent steps are successful
        time = None
        with open(kaslr_file_path) as f:
            for line in f:
                time_match = re.match(KASLR_TIME_REGEX, line)
                if time_match:
                    time = time_match.group("kaslr_time")
            if time is None:
                print("Invalid file", time, kaslr_file_path)
                exit(1)
        run_data["kaslr_time"] = s_to_ms(time)

        # parse shadow leak results
        shadow_rb_time = None
        shadow_pm_time = None
        shadow_find_time = None
        shadow_leak_time = None
        shadow_leak_success = False
        shadow_leak_partial_success = False
        with open(shadow_file_path) as f:
            for line in f:
                rb_time_match = re.match(RB_TIME_REGEX, line)
                pm_time_match = re.match(PM_TIME_REGEX, line)
                shadow_time_match = re.match(SHADOW_FIND_REGEX, line)
                shadow_leak_match = re.match(SHADOW_LEAK_REGEX, line)
                if rb_time_match:
                    shadow_rb_time = rb_time_match.group("time")
                elif pm_time_match:
                    shadow_pm_time = pm_time_match.group("time")
                elif shadow_time_match:
                    shadow_find_time = shadow_time_match.group("time")
                elif shadow_leak_match:
                    shadow_leak_time = shadow_leak_match.group("time")
                elif line.startswith(ROOT_HASH):
                    shadow_leak_success = True
                elif line.startswith(ROOT_HASH[:5]):
                    line = line[:-1]
                    # print(line)
                    # print(ROOT_HASH)
                    shadow_leak_partial_success_errors = 0
                    for i, c in enumerate(line):
                        if i >= len(ROOT_HASH) or c != ROOT_HASH[i]:
                            # print("!", end="")
                            shadow_leak_partial_success_errors += 1
                        # else:
                        #     print(" ", end="")
                    # print()
                    if shadow_leak_partial_success_errors <= 6:
                        shadow_leak_partial_success = True

        run_data["shadow_rb_time"] = s_to_ms(shadow_rb_time)
        run_data["shadow_pm_time"] = s_to_ms(shadow_pm_time)
        run_data["shadow_find_time"] = s_to_ms(shadow_find_time)
        run_data["shadow_leak_time"] = s_to_ms(shadow_leak_time)
        run_data["shadow_leak_success"] = shadow_leak_success
        run_data["shadow_leak_partial_success"] = shadow_leak_partial_success

        # parse bandwidth leak results
        bandwidth_secret_size = None
        bandwidth_leak_time = None
        bandwidth_leak_mistakes = False
        with open(bandwidth_file_path) as f:
            for line in f:
                bandwidth_secret_size_match = re.match(
                    BANDWIDTH_SECRET_SIZE_REGEX, line)
                bandwidth_leak_time_match = re.match(
                    BANDWIDTH_LEAK_REGEX, line)
                bandwidth_leak_mistakes_match = re.match(
                    BANDWIDTH_MISTAKES_REGEX, line)
                if bandwidth_secret_size_match:
                    bandwidth_secret_size = bandwidth_secret_size_match.group(
                        "size")
                elif bandwidth_leak_time_match:
                    bandwidth_leak_time = bandwidth_leak_time_match.group(
                        "time")
                elif bandwidth_leak_mistakes_match:
                    bandwidth_leak_mistakes = bandwidth_leak_mistakes_match.group(
                        "count")
        run_data["bandwidth_secret_size"] = int(
            bandwidth_secret_size) if bandwidth_secret_size is not None else None
        run_data["bandwidth_leak_time"] = s_to_ms(bandwidth_leak_time)
        run_data["bandwidth_leak_mistakes"] = int(
            bandwidth_leak_mistakes) if bandwidth_leak_mistakes is not None else None

        data_set.append(run_data)

    return data_set


def analyze_data(data_set):
    results = {}

    results["num_runs"] = len(data_set)

    # check if subsequent steps worked to find out whether KASLR succeded
    success_count = 0
    for entry in data_set:
        if entry['shadow_leak_time'] is not None or entry['bandwidth_leak_time'] is not None:
            success_count += 1
    results["kaslr_success_rate"] = success_count / len(data_set)

    # calculate median KASLR breaking time
    kaslr_times = [elem['kaslr_time'] for elem in data_set]
    results["kaslr_median_time"] = statistics.median(kaslr_times)

    # calculate the median time to reload buffer and physmap and total success rate
    rb_times = [elem['shadow_rb_time']
                for elem in data_set if elem['shadow_rb_time'] is not None]
    pm_times = [elem['shadow_pm_time']
                for elem in data_set if elem['shadow_pm_time'] is not None]
    success_count = 0
    for entry in data_set:
        if entry['shadow_leak_time'] is not None:
            success_count += 1
        if entry['bandwidth_leak_time'] is not None:
            success_count += 1
    results["rb_median_time"] = statistics.median(rb_times)
    results["pm_median_time"] = statistics.median(pm_times)
    results["pm_rb_success_rate"] = success_count / len(data_set) / 2

    # calculate time to shadow and success rate
    shadow_times = [elem['shadow_find_time'] + elem['shadow_leak_time']
                    for elem in data_set if elem['shadow_rb_time'] is not None and elem['shadow_leak_time'] is not None]
    success_count = 0
    for entry in data_set:
        if entry['shadow_leak_success']:
            success_count += 1
    results["shadow_success_rate"] = success_count / len(data_set)
    success_count = 0
    for entry in data_set:
        if entry['shadow_leak_success'] or entry['shadow_leak_partial_success']:
            success_count += 1
    results["shadow_partial_success_rate"] = success_count / len(data_set)
    results["shadow_median_time"] = statistics.median(shadow_times)

    # calculate the median bandwidth and accuracy
    bandwidth_leak_time = [elem['bandwidth_leak_time'] for elem in data_set if elem['bandwidth_leak_time'] is not None]
    bandwidth_leak_mistakes = [elem['bandwidth_leak_mistakes']
                for elem in data_set if elem['bandwidth_leak_mistakes'] is not None]
    results["bandwidth_leak_rate"] = round((LEAK_SIZE / 1024) / (statistics.median(bandwidth_leak_time) / 1000), 1)
    results["bandwidth_leak_accuracy"] = round((LEAK_SIZE - statistics.median(bandwidth_leak_mistakes)) / LEAK_SIZE, 3)

    return results


def main():
    for host in os.listdir(DATA_DIR_PATH):
        if host == ".archive":
            continue

        data_set = collect_data(DATA_DIR_PATH, host)
        results = analyze_data(data_set)

        print(f"{host}:")
        print(f"  Total number of runs: {results['num_runs']}")

        print(f"  Median KASLR time: {results['kaslr_median_time']}ms")
        print(f"  KASLR success percentage: {100 * results['kaslr_success_rate']}%")

        print(f"  Median RB Time: {results['rb_median_time']}ms")
        print(f"  Median PM Time: {results['pm_median_time']}ms")
        print(f"  RB/PM success percentage: {100 * results['pm_rb_success_rate']}%")

        print(f"  Median Time to Shadow: {results['shadow_median_time'] / 1000}s")
        print(f"  Shadow success percentage: {100 * results['shadow_success_rate']}%")
        print(f"  Shadow partial success percentage: {100 * results['shadow_partial_success_rate']}%")
        
        print(f"  Leak Rate: {results['bandwidth_leak_rate']}KiB/s")
        print(f"  Leak Accuracy: {100 * results['bandwidth_leak_accuracy']}%")

        print()


if __name__ == "__main__":
    main()
