// SPDX-License-Identifier: GPL-3.0-only
#include <stdio.h>
#include <syscall.h>
#include <err.h>

#include "../spec_ctrl_ioctl.h"

int main(int argc, char *argv[])
{
    spec_ctrl_init();

    // create a tripwire that we can trigger
    int cpu = 2;
    unsigned long spec_ctrl = 0;
    if (spec_ctrl_get(cpu, &spec_ctrl))
    {
        err(1, "spec_ctrl_get");
    }
    printf("core %d: 0x%lx\n", cpu, spec_ctrl);

    if (spec_ctrl_set(cpu, 0))
    {
        err(1, "spec_ctrl_get");
    }

    unsigned long spec_ctrl_post = 0;
    if (spec_ctrl_get(cpu, &spec_ctrl_post))
    {
        err(1, "spec_ctrl_get");
    }
    printf("core %d: 0x%lx\n", cpu, spec_ctrl_post);

    if (spec_ctrl_set(cpu, spec_ctrl))
    {
        err(1, "spec_ctrl_get");
    }

    return 0;
}
