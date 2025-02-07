// SPDX-License-Identifier: GPL-3.0-only
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/proc_fs.h>

#include "./spec_ctrl_ioctl.h"

typedef struct spec_ctxt
{
    unsigned int req;
    spec_ctrl_args_t args;

    long ret_val;
} spec_ctxt_t;

static struct proc_dir_entry *procfs_file;

static bool cond_core(int cpu, void *info)
{
    spec_ctxt_t *ctxt = info;
    return cpu == ctxt->args.cpu;
}

static void run_on_core(void *info)
{
    spec_ctxt_t *ctxt = info;

    switch (ctxt->req)
    {
    case IOCTL_GET_SPEC_CTRL:
    {
        ctxt->args.value = this_cpu_read(x86_spec_ctrl_current);
        break;
    }
    case IOCTL_SET_SPEC_CTRL:
    {
        this_cpu_write(x86_spec_ctrl_current, ctxt->args.value);
        native_wrmsrl(MSR_IA32_SPEC_CTRL, ctxt->args.value);
        break;
    }
    default:
        ctxt->ret_val = -ENOENT;
        break;
    }

    pr_info("cpu[%d].SPEC_CTRL = 0x%lx\n", ctxt->args.cpu, ctxt->args.value);

    return;
}

static long handle_ioctl(struct file *filp, unsigned int req,
                         unsigned long argp)
{
    spec_ctxt_t ctxt = {
        .req = req,
        .ret_val = 0,
    };

    // get the arguments
    if (copy_from_user(&ctxt.args, (void *)argp, sizeof(ctxt.args)) != 0)
    {
        return -EFAULT;
    }

    on_each_cpu_cond(cond_core, run_on_core, &ctxt, 1);

    if (copy_to_user((void *)argp, &ctxt.args, sizeof(ctxt.args)) != 0)
    {
        return -EFAULT;
    }

    return ctxt.ret_val;
}

static struct proc_ops pops = {
    .proc_ioctl = handle_ioctl,
    .proc_open = nonseekable_open,
#ifdef no_llseek
    .proc_lseek = no_llseek,
#else
    .proc_lseek = noop_llseek,
#endif
};

static int __init spec_ctrl_init(void)
{
    pr_info("hello\n");

    procfs_file = proc_create(PROC_SPEC_CTRL, 0, NULL, &pops);

    return 0;
}

static void __exit spec_ctrl_exit(void)
{
    proc_remove(procfs_file);

    pr_info("good bye\n");
}

module_init(spec_ctrl_init);
module_exit(spec_ctrl_exit);
MODULE_LICENSE("GPL");
