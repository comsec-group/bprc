# Branch Privilege Injection: Compromising Spectre v2 Hardware Mitigations by Exploiting Branch Predictor Race Conditions

|                          |                                              |
| ------------------------ | -------------------------------------------- |
| **Authors:**             | Sandro Rüegge, Johannes Wikner, Kaveh Razavi |
| **Organization:**        | ETH Zürich                                   |
| **Published at:**        | 34th USENIX Security Symposium               |
| **Webpage:**             | TODO                                         |
| **Security Advisories:** | TODO                                         |

# Introduction

We introduce Branch Predictor Race Conditions (BPRC), an event-misordering effect of asynchronous branch predictors.
Our work demonstrates multiple variants of BPRC that violate security assumptions on Intel processors, enabling cross-privilege and even cross barrier BTI attacks.
This repository contains the paper artifacts to reproduce all presented results.


> [!CAUTION]
> Some experiments load kernel modules that allow unprivileged users to execute arbitrary code in the kernel.
> This is an open door for attackers and can potentially damage the integrity of your system
> 
> **Do not run on a machine that is security sensitive or where you cannot afford to loose data/availability.**

> [!CAUTION]
> The ansible playbooks modify the target systems without requesting confirmation.
> Ansible will also reboot the target systems.
> 
> **Do not run them on your localhost.**
> 
# Requirements

**OS:** Ubuntu (should run on 20.04, 22.04 and 24.04)

## Microarchitectures

| Vendor | CPU              | Codename            | Code  | Microarchitecture | experiment_core | experiment_march |
| ------ | ---------------- | ------------------- | ----- | ----------------- | --------------: | ---------------- |
| Intel  | Core i7-14700K   | Raptor Lake Refresh | RPL-R | Raptor Cove       |               2 | MARCH_GOLDEN     |
| Intel  | Core i7-14700K   | Raptor Lake Refresh | RPL-R | Gracemont         |              16 | MARCH_GRACE      |
| Intel  | Xeon Silver 4510 | Sapphire Rapids     | SPR   | Golden Cove       |               2 | MARCH_GOLDEN     |
| Intel  | Core i7-13700K   | Raptor Lake         | RPL   | Raptor Cove       |               2 | MARCH_GOLDEN     |
| Intel  | Core i7-13700K   | Raptor Lake         | RPL   | Gracemont         |              16 | MARCH_GRACE      |
| Intel  | Core i7-12700K   | Alder Lake          | ADL   | Golden Cove       |               2 | MARCH_GOLDEN     |
| Intel  | Core i7-12700K   | Alder Lake          | ADL   | Gracemont         |              16 | MARCH_GRACE      |
| Intel  | Core i7-11700K   | Rocket Lake         | RKL   | Cypress Cove      |               2 | MARCH_CYPRESS    |
| Intel  | Core i7-10700K   | Comet Lake          | CML   | Skylake           |               2 | MARCH_SKYLAKE    |
| Intel  | Core i9-9900K    | Coffee Lake Refresh | CFL-R | Skylake           |               2 | MARCH_SKYLAKE    |
| AMD    | Ryzen 9 9900X    | Granite Ridge       | -     | Zen 5             |               2 | MARCH_ZEN5       |
| AMD    | Ryzen 7 7700X    | Raphael             | -     | Zen 4             |               2 | MARCH_ZEN4       |
| ARM    | Google Tensor    | Whitechapel         | -     | Cortex-X1         |               4 | MARCH_ARMv8      |
| ARM    | Google Tensor    | Whitechapel         | -     | Cortex-A76        |               6 | MARCH_ARMv8      |

Your CPU might have different numbers of cores and thus the Gracemont cores might be at a different index.
The `experiment_core` is used with `taskset -c <experiment_core>` to determine the core to run on.

## Dependencies

### Ansible

The experiments were originally run using ansible and the manual execution descriptions are provided for convenience.
To run the experiments with ansible you can install ansible as follows.

```bash
sudo apt install python3-venv

python3 -m venv .venv
source .venv/bin/activate

pip install ansible
```

If you already have an ansible inventory you can use the host names from your inventory.
Otherwise you can use a hostname or IP address where the READMEs use `<hostname>` and add an argument with `-i <hostname>,` (**note the comma**).

```bash
ansible-playbook -i example.com, run.yaml -e host=example.com
```

### Analysis

Our experiments provide python code to analyze the results.
Some of the scripts require you to have matplotlib installed.
```bash
sudo apt install python3 python3-pip
pip install matplotlib
```

### x86_64
```bash
sudo apt install build-essential clang linux-headers-$(uname -r) msr-tools
```

### armv8
The [Android NDK r27c](https://dl.google.com/android/repository/android-ndk-r27c-linux.zip) binary tools directory (`toolchains/llvm/prebuilt/linux-x86_64/bin/`) needs to be available in your `$PATH`.

### Metadata

We store server metadata in the file [exp_metadata.py](./experiments/exp_metadata.py).
There is a dummy entry for the manual executions but if you use the ansible scripts on your own hosts you will need to add your servers to the `SERVER_MAP` in this file.

# Overview

Each experiment has their own `README.md` describing how to run the experiment and what the expected outcomes are.

### Folder Structure
```
bprc
|- ansible                     # Ansible tools to run experiments consistently
\- experiments                 # Experiments from the paper
   |- exp-benchmark-mitigations    # Mitigation performance evaluation
   |- exp-bhi-dis-s                # Intel's BHI_DIS_S behavior
   |- exp-btb-delay                # Branch predictor insertion delays on x86
   |- exp-end2end                  # End-to-end attack
   |- exp-ibp-insertion            # IBP insertion behavior
   |- exp-leak-hypervisor          # Hypervisor vulnerability test
   |- exp-leak-ibpb                # IBPB vulnerability test
   |- exp-leak-rounds              # BPI reliability
   |- exp-leak-supervisor          # Supervisor vulnerability test
   |- exp-leak-supervisor-discern  # Supervisor vulnerability predictor attribution
   |- exp-syscall-split            # BPI syscall delay
   \- uarch-research-fw            # framework with general utils
```

### Experiments by paper section

| ID  | Section     | Reference                         | Folder Name                                                              |
| --- | ----------- | --------------------------------- | ------------------------------------------------------------------------ |
| -   | A.3.2       | Artifact Appendix Basic Test      | [exp-test-setup             ](./experiments/exp-test-setup            )  |
| E1  | Section 5.1 | Figure 2                          | [exp-btb-delay              ](./experiments/exp-btb-delay              ) |
| E2  | Section 5.2 | Table 2, $BPRC_{U \rightarrow K}$ | [exp-leak-supervisor        ](./experiments/exp-leak-supervisor        ) |
| E3  | Section 5.3 | $BPRC_{G \rightarrow H}$          | [exp-leak-hypervisor        ](./experiments/exp-leak-hypervisor        ) |
| E4  | Section 5.3 | $BPRC_{IBPB}$                     | [exp-leak-ibpb              ](./experiments/exp-leak-ibpb              ) |
| E5  | Section 6.1 | Observation (O4)                  | [exp-ibp-insertion          ](./experiments/exp-ibp-insertion          ) |
| E6  | Section 6.1 | Observation (O5)                  | [exp-leak-supervisor-discern](./experiments/exp-leak-supervisor-discern) |
| E7  | Section 6.2 | Figure 8                          | [exp-syscall-split          ](./experiments/exp-syscall-split          ) |
| E8  | Section 7.1 | Figure 9                          | [exp-leak-rounds            ](./experiments/exp-leak-rounds            ) |
| E9  | Section 7.1 | Table 3                           | [exp-bhi-dis-s              ](./experiments/exp-bhi-dis-s              ) |
| E10 | Section 8   | Textual                           | [exp-end2end                ](./experiments/exp-end2end                ) |
| -   | Section 8   | Table 4                           | summary of previous experiments                                          |
| E11 | Section 9   | Table 5                           | [exp-benchmark-mitigations  ](./experiments/exp-benchmark-mitigations  ) |

# Citation

```bibtex
@inproceedings{TODO,
  title     = {{Branch Privilege Injection}: Compromising Spectre v2 Hardware
               Mitigations by Exploiting Branch Predictor Race Conditions},
  author    = {Sandro Rüegge, Johannes Wikner and Kaveh Razavi},
  booktitle = {34th {USENIX} Security Symposium ({USENIX} Security 25)},
  year      = {2025}
}
```
