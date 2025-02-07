# Framework

Libraries and kernel modules for microarchitectural research.

## Kernel modules

| Module         | Purpose                                                      |
| -------------- | ------------------------------------------------------------ |
| kmod_ap        | run arbitrary code in kernel space                           |
| kmod_secret    | create a random secret in kernel for memory leak experiments |
| kmod_spec_ctrl | modify spec ctrl MSR and update the kernel's copy            |

## Libraries

| Library  | Purpose                                |
| -------- | -------------------------------------- |
| bp_tools | simple branch history control          |
| branch   | extended branch history control        |
| lib      | fence instructions                     |
| log      | log formatting                         |
| memtools | tools to place code snippets in memory |
| msr      | msr read/write helpers                 |
| pfc      | read out performance counters on x86   |
| rb_tools | flush+reload implementation            |
| stats    | statistics helpers                     |

