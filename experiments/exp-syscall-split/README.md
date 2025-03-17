# Investigate BPI breaking points

Experiment to determine the effect of timing on BPI.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

> [!TIP]
> This experiment runs very slow on some cores (e.g., Xeon Silver 4510, Xeon Silver 4514Y).
> You can reduce the number of rounds in `main.c` and `analyze.py` by changing `NUM_ROUNDS` to something lower.
> Values of 2000 (`main.c`) and 1000 (`analyze.py`), respectively, work well.
> While this will lead to more noise in the plots, the effects are still observable.

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook ../../ansible/run.yaml -e host=<hostname> -e experiment=exp-bhi-dis-s \
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

The figure as we presented it in our paper will be placed at [syscall_split.pdf](../../figures/syscall_split.pdf).

### Expected Results

We expect to see the number of hits in the kernel decrease with increasing number of NOPs while the number of hits in user increases with increasing number of NOPs.