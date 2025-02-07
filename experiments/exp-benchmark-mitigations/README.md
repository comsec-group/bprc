
# Mitigation Performance Benchmarks

We measured the performance impact for the suggested mitigations using [UnixBench](https://github.com/kdlucas/byte-unixbench/tree/2c29fe37ef23d9ece3a19e7215b966b7d199ede1) and [lmbench](https://github.com/intel/lmbench/tree/701c6c35b0270d4634fb1dc5272721340322b8ed).
The specific code versions we used are also available as zip files in [files](./files) to ensure availability.


## Reproduce

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

The output is stored in [out](./out).

## Analyze

```bash
python3 analyze.py
```

The table as we presented it in our paper is placed at [mitigations.tex](../../tables/mitigations.tex).
The results for the Alder Lake microcode mitigation are printed to the console.


### Expected Results

We observed a slowdown for all mitigations. The microcode performs best, `IPRED_DIS_S` second and retpoline third.