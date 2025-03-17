# Investigate BPI reliability

Test reliability of BPI when repeatedly attacking the same victim branch without modifying history with and without `BHI_DIS_S`.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

> [!IMPORTANT]
> The `BHI_DIS_S` mitigation is only available on newer processors.
> Of the evaluated systems, only Raptor Lake Refresh, Sapphire Rapids, Raptor Lake and Alder Lake support it.

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-leak-rounds \
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

The figure as we presented it in our paper will be placed at [leak_rounds.pdf](../../figures/leak_rounds.pdf).

### Expected Results

With only eIBRS we expect a decreasing success rate over successive repetitions.
When eIBRS and `BHI_DIS_S` are enabled we expect the success rate to increase over the first two repetitions and then stay high (>90%).