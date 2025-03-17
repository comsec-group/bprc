# Investigate `BHI_DIS_S`

Experiment to determine how `BHI_DIS_S` is implemented on recent Intel processors.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

> [!IMPORTANT]
> The `BHI_DIS_S` mitigation is only available on newer processors.
> Of the evaluated systems, only Raptor Lake Refresh, Sapphire Rapids, Raptor Lake and Alder Lake support it.

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-bhi-dis-s \
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

# run (sudo required for Linux PFC)
sudo make run CORE=$experiment_core | tee "$OUT_DIR/run.out"
```


## Analyze

```bash
python3 analyze.py
```

The table as we presented it in our paper will be placed at [bhi-dis-s.tex](../../tables/bhi-dis-s.tex).

### Expected Results

We expect a low number of mispredictions when there is no mitigation active (<10%) and a high number of mispredictions when `BHI_DIS_S` is active.