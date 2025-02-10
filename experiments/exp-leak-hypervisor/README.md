
# Investigate $BPRC_{G \rightarrow H}$

Experiment to determine if $BPRC_{G \rightarrow H}$ is possible on recent Intel processors.


## Reproduce

Replace `<experiment_core>` and `<experiment_march>` according to the [Microarchitectures](../../README.md#microarchitectures) table. The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages, ..).

```bash
ansible-playbook run.yaml -e host=<hostname> \
    -e experiment_core=<experiment_core>
```


### Manual

**Setup code in Linux selftest**

```bash
export experiment_core=<experiment_core>
export OUT_DIR="$(pwd)/out/manual"

# load kmod_ap kernel module
make -C ../uarch-research-fw/kmod_ap/ install
make -C ../uarch-research-fw/kmod_spec_ctrl/ install

# get linux v6.6
git clone https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git --branch v6.6 --single-branch --depth 1

# link our into the selftest code framework
ln -s "$(pwd)/../uarch-research-fw" linux/tools/testing/selftests/kvm/lib/uarch-research-fw
ln -s "$(pwd)/../uarch-research-fw" linux/tools/testing/selftests/kvm/include/uarch-research-fw

# copy our Makefile and code into the selftests 
cp Makefile linux/tools/testing/selftests/kvm/
mkdir linux/tools/testing/selftests/kvm/x86_64/rev_eng/
cp main.c linux/tools/testing/selftests/kvm/x86_64/rev_eng/

# copy the current kernel config to build the headers
cp /boot/config-$(uname -r) linux/.config

# update config with default options
cd linux
yes "" | make oldconfig

# build
make headers;
cd tools/testing/selftests/kvm;
make

# create output directory
mkdir -p "$OUT_DIR"

# save metadata
mkdir -p "$OUT_DIR"
cat > "$OUT_DIR/metadata.json" <<EOF
{
    "hostname": "manual",
    "experiment_core": $experiment_core
}
EOF

# run
taskset -c $experiment_core ./x86_64/rev_eng/main $experiment_core | tee "$OUT_DIR/run.out"

cd ../../../../../
```

## Analyze

```bash
python3 analyze.py
```

The results are printed to the console.

### Expected Results

We expect above-noise numbers on all evaluated Intel processors for at least one instruction type except on Gracemont where we observed no results above noise.
