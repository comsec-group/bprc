# Investigate $BPRC_{U \rightarrow K}$

Experiment to determine if $BPRC_{U \rightarrow K}$ is possible on recent Intel processors.
Additionally, we empirically tested whether `IPRED_DIS_S` and `RSBA_DIS_S` can be used to mitigate branch privilege injection.
These mitigations are only available on 12th gen and newer Intel processors.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).


Experiment to determine if $BPRC_{U \rightarrow K}$  is possible
```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-leak-supervisor \
    -e experiment_core=<experiment_core> \
    -e experiment_march=<experiment_march>
```

We used the following commands to test potential mitigations on 12th gen and newer Intel processors.

Empirically verify that `IPRED_DIS_S` prevents $BPRC_{U \rightarrow K}$.
```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-leak-supervisor \
    -e experiment_core=<experiment_core> \
    -e experiment_march=<experiment_march> \
    -e experiment_args=0x11
```

Empirically verify that `RRSBA_DIS_S` prevents $BPRC_{U \rightarrow K}$ for returns.
```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-leak-supervisor \
    -e experiment_core=<experiment_core> \
    -e experiment_march=<experiment_march> \
    -e experiment_args=0x41
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

We used the following commands to test potential mitigations on 12th gen and newer Intel processors.

To test `IPRED_DIS_S`.
```bash
make run CORE=$experiment_core CUSTOM_ARGS="0x11" | tee "$OUT_DIR/run.out"
```

To test `RSBA_DIS_S`.
```bash
make run CORE=$experiment_core CUSTOM_ARGS="0x41" | tee "$OUT_DIR/run.out"
```


## Analyze

```bash
python3 analyze.py
```

The table as we presented it in our paper will be placed at [eibrs-leak.tex](../../tables/eibrs-leak.tex).

### Expected Results

For the initial experiment we expect a success percentage above the noise level on at least two of the three tested instruction for all Intel [microarchitectures](../../README.md#microarchitectures) presented in our evaluation.

#### `IPRED_DIS_S`

With `IPRED_DIS_S` enabled we expect no signal anymore on any Intel microarchitecture. 

#### `RRSBA_DIS_S`

With `RRSBA_DIS_S` enabled we expect no signal **on returns** anymore on any Intel microarchitecture. 
