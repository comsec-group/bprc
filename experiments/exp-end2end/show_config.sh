#!/bin/env bash

set -e
set -x

grep -m1 'model name' /proc/cpuinfo
grep -m1 'microcode' /proc/cpuinfo
lsb_release -a
uname -r
free -h
cat /proc/sys/vm/nr_hugepages
cat /sys/kernel/mm/transparent_hugepage/enabled
cat /sys/devices/system/cpu/vulnerabilities/spectre_v2
cat /proc/cmdline
