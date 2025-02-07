// SPDX-License-Identifier: GPL-3.0-only
#include <linux/module.h>
#include <linux/proc_fs.h>
#include "./secret_ioctl.h"

static struct proc_dir_entry *procfs_file;
unsigned char secret_data[SECRET_DATA_SIZE];

static long handle_ioctl(struct file *filp, unsigned int req, unsigned long argp)
{
    switch (req)
    {
    case IOCTL_SECRET_GET_ADDR:
        unsigned long secret_addr = (unsigned long)&(secret_data[0]);
        if (copy_to_user((void *)argp, &secret_addr, sizeof(secret_addr)) != 0)
        {
            return -EFAULT;
        }
        break;
    case IOCTL_SECRET_GET_DATA:
        secret_get_data_t args;
        if (copy_from_user(&args, (void *)argp, sizeof(args)) != 0)
        {
            return -EFAULT;
        }
        if (args.size < SECRET_DATA_SIZE)
        {
            return -EINVAL;
        }
        if (copy_to_user((void *)args.buffer, secret_data, SECRET_DATA_SIZE) != 0)
        {
            return -EFAULT;
        }
        break;
    default:
        return -ENOIOCTLCMD;
    }
    return 0;
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

static int mod_secret_init(void)
{
    // fill the secret array with some data
    get_random_bytes(secret_data, sizeof(secret_data));
    pr_info("secret@%012lx:\n", (unsigned long)secret_data);
    for (int i = 0; i < SECRET_DATA_SIZE / 8; ++i)
    {
        pr_debug("0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x,\n",
                secret_data[i * 8 + 0],
                secret_data[i * 8 + 1],
                secret_data[i * 8 + 2],
                secret_data[i * 8 + 3],
                secret_data[i * 8 + 4],
                secret_data[i * 8 + 5],
                secret_data[i * 8 + 6],
                secret_data[i * 8 + 7]);
    }

    procfs_file = proc_create(PROC_SECRET, 0, NULL, &pops);

    return 0;
}

static void mod_secret_exit(void)
{
    // make sure that the secret is not accidentally left in memory
    memset(secret_data, 0, SECRET_DATA_SIZE);

    proc_remove(procfs_file);
}

module_init(mod_secret_init);
module_exit(mod_secret_exit);

MODULE_LICENSE("GPL");
