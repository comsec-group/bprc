# Investigate $BPRC_{IBPB}$

Experiment to determine if $BPRC_{IBPB}$ is possible on recent Intel processors.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-leak-ibpb \
    -e experiment_core=<experiment_core>
```


### Manual

```bash
export experiment_core=<experiment_core>
export OUT_DIR=./out/manual

# load kmod_ap kernel module
make -C ../uarch-research-fw/kmod_ap/ install

# build
make clean
make

# save metadata
mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/metadata.json" <<EOF
{
    "hostname": "manual",
    "experiment_core": $experiment_core
}
EOF

# run
make run CORE=$experiment_core | tee "$OUT_DIR/run.out"
```


## Analyze

```bash
python3 analyze.py
```

The results are printed to the console.

### Expected Results

We expect above-noise numbers on all evaluated Intel processors for at least one instruction type except on Gracemont where we observed no results above noise.
