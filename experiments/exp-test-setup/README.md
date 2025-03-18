# Test Setup

Experiment to determine if performance counters can be read, flush+reload works and kernel modules are loaded.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-test-setup \
    -e experiment_core=<experiment_core> \
    -e experiment_march=<experiment_march>
```


### Manual

```bash
export experiment_march=<experiment_march>
export experiment_core=<experiment_core>
export OUT_DIR=./out/manual

# load kmod_ap kernel module
make -C ../uarch-research-fw/kmod_ap/ install

# build
make clean
make MARCH=$experiment_march

# create output directory
mkdir -p $OUT_DIR

# run (sudo required for Linux PFC)
sudo make run CORE=$experiment_core | tee "$OUT_DIR/run.out"
```

### Expected Results

The program confirms that Performance counters, Reload buffer and Kernel module ap are working.
The output should look similar to this:
```log
taskset -c 2 ./main
[-] BR_INST_RETIRED__ALL_BRANCHES  sum: 771070, avg: 77.107000
[-] Performance counters: OK
[-] Reload buffer cleared: 00000
[-] Reload buffer accessed: 10000
[-] Reload buffer: OK
[-] Kernel module ap: OK
```