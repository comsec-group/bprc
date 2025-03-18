
# Mitigation Performance Benchmarks

We measured the performance impact for the suggested mitigations using [UnixBench](https://github.com/kdlucas/byte-unixbench/tree/2c29fe37ef23d9ece3a19e7215b966b7d199ede1) and [lmbench](https://github.com/intel/lmbench/tree/701c6c35b0270d4634fb1dc5272721340322b8ed).
The specific code versions we used are also available as zip files in [files](./files) to ensure availability.


## Reproduce

The output is stored in [out](./out).

### Ansible

> [!WARNING]
> The ansible playbooks modify the system without requesting confirmation (ex. GRUB config, install packages).

Hosts that support Intel's `IPRED_DIS_S` (RPL, SPR, ADL).
```bash
ansible-playbook run.yaml -e host=<hostname> -e baseline=True -e retpoline=True -e ipred=True
```

Hosts that do not support Intel's `IPRED_DIS_S` (RKL, CML, CFL-R).
```bash
ansible-playbook run.yaml -e host=<hostname> -e baseline=True -e retpoline=True
```

For microcode benchmarking, we installed the microcode Intel provided us with on the Alder Lake server and ran the baseline benchmark again.
```bash
ansible-playbook run.yaml -e host=<hostname> -e baseline=True
```

### Manual

While we did not run these benchmarks manually, we provide an outline of how to do so for the sake of completeness.
The artifacts include the zipped versions of the unixbench and lmbench repository version we used for the evaluation (at [./files/](./files/)).

#### configuration

We ran the benchmark in different configurations as per [Ansible](#ansible).
The following lines describe the changes required for each configuration.

| configuration | GRUB                                                  | extra         |
| ------------- | ----------------------------------------------------- | ------------- |
| baseline      | GRUB_CMDLINE_LINUX_DEFAULT=quiet                      |               |
| retpoline     | GRUB_CMDLINE_LINUX_DEFAULT=quiet spectre_v2=retpoline |               |
| ipred         | GRUB_CMDLINE_LINUX_DEFAULT=quiet                      | command below |

Update the grub variable from the table in `/etc/default/grub`, run `sudo update-grub` and `sudo reboot`.
For the ipred configuration, we additionally enable the mitigation using the following command.
```bash
for i in $(seq 0 $(($(nproc) - 1))); do sudo wrmsr -p $i 0x48 0x411; echo -n "core[$i]: "; sudo rdmsr -p $i 0x48; done
```

Replace `<configuration>` with the configuration you are currently testing

#### unixbench

```bash
configuration=<configuration> # baseline, retpoline or ipred
OUT_DIR="out/$(hostname)/$configuration/unixbench"

# unpack unixbench
cd files
unzip byte-unixbench-2c29fe37ef23d9ece3a19e7215b966b7d199ede1.zip

# build it
cd byte-unixbench-2c29fe37ef23d9ece3a19e7215b966b7d199ede1/UnixBench/
make

# run it
./Run

# create output directory
cd ../../../
mkdir -p "$OUT_DIR"

# copy the results
cp files/byte-unixbench-2c29fe37ef23d9ece3a19e7215b966b7d199ede1/UnixBench/results/* "$OUT_DIR"
```

#### lmbench

```bash
configuration=<configuration> # baseline, retpoline or ipred
OUT_DIR="out/$(hostname)/$configuration/lmbench"

# unpack lmbench
cd files
unzip lmbench-701c6c35b0270d4634fb1dc5272721340322b8ed.zip

# build it
cd lmbench-701c6c35b0270d4634fb1dc5272721340322b8ed/
make

# if the build fails, insert the following two lines right before "# now go ahead and build everything!" in scripts/build
LDLIBS="${LDLIBS} -ltirpc"
CFLAGS="${CFLAGS} -I/usr/include/tirpc"
# then try make again

# configure
cp ../CONFIG.generic "bin/x86_64-linux-gnu/CONFIG.$(hostname)"

# run it
make rerun

# create output directory
cd ../../
mkdir -p "$OUT_DIR"

# copy the results
cp files/lmbench-701c6c35b0270d4634fb1dc5272721340322b8ed/results/x86_64-linux-gnu/* "$OUT_DIR"
```

## Analyze

```bash
python3 analyze.py
```

The table as we presented it in our paper is placed at [mitigations.tex](../../tables/mitigations.tex).
The results for the Alder Lake microcode mitigation are printed to the console.


### Expected Results

We observed a slowdown for all mitigations. The microcode performs best, `IPRED_DIS_S` second and retpoline third.