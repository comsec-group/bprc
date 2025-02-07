// SPDX-License-Identifier: GPL-3.0-only
#define PROC_SPEC_CTRL "spec_ctrl"
#define IOCTL_GET_SPEC_CTRL 0x3134000
#define IOCTL_SET_SPEC_CTRL 0x3134001

typedef struct spec_ctrl_args
{
    int cpu;

    unsigned long value;
} spec_ctrl_args_t;

#ifndef __KERNEL__
#include <stddef.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <err.h>
static int fd_spec_ctrl;

#define spec_ctrl_init()                                        \
    do                                                          \
    {                                                           \
        fd_spec_ctrl = open("/proc/" PROC_SPEC_CTRL, O_RDONLY); \
        if (fd_spec_ctrl <= 0)                                  \
            err(1, "spec_ctrl_init");                           \
    } while (0)

static inline int spec_ctrl_get(int cpu, unsigned long *value_ptr)
{
    struct spec_ctrl_args args = {
        .cpu = cpu,
    };
    int retval = ioctl(fd_spec_ctrl, IOCTL_GET_SPEC_CTRL, &args);

    *value_ptr = args.value;
    return retval;
}

static inline int spec_ctrl_set(int cpu, unsigned long value)
{
    struct spec_ctrl_args args = {
        .cpu = cpu,
        .value = value,
    };
    return ioctl(fd_spec_ctrl, IOCTL_SET_SPEC_CTRL, &args);
}

#endif
