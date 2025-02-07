
# Investigate IBP Insertion

Experiment to determine if a prediction is inserted into the BTB and IBP the first time it is seen.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-ibp-insertion \
    -e experiment_core=<experiment_core>
```


### Manual

```bash
export experiment_core=<experiment_core>
export OUT_DIR=./out/manual

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

We ran this experiment on the Intel performance (P-Cores), Cypress Cove and Skylake cores.
For 'Random History' we expect many hits on the BTB (>90%) and very few on the IBP (<1%>).
For 'Matching History' we expect few hits on the BTB (<10%>) and many hits on the IBP (>90%).