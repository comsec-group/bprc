#!/bin/env bash
set -x

grep -m1 'model name' /proc/cpuinfo
lsb_release -a | grep "Descr"
uname -r
cat /sys/devices/system/cpu/vulnerabilities/spectre_v2
cat /proc/cmdline

cat /etc/shadow
sudo -l
