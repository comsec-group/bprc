# Investigate source of $BPRC_{U \rightarrow K}$

Experiment to determine if $BPRC_{U \rightarrow K}$ is caused by the BTB or IBP on Intel.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-leak-supervisor-discern \
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

# save metadata
mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/metadata.json" <<EOF
{
    "hostname": "manual",
    "experiment_core": $experiment_core,
    "experiment_march": "$experiment_march"
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

We expect to see hits on the BTB above the noise level while there are no hits above noise level on the IBP.