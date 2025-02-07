// SPDX-License-Identifier: GPL-3.0-only
#ifndef __KERNEL__
#include <err.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/ioctl.h>
#endif

#define PROC_SECRET "secret"
#define IOCTL_SECRET_GET_ADDR 0x156124 /* an arbitrarily picked request id */
#define IOCTL_SECRET_GET_DATA 0x156125 /* an arbitrarily picked request id */

#define SECRET_DATA_SIZE (256 * 4096) /* 1MB */

typedef struct secret_get_data
{
    unsigned char *buffer;
    size_t size;
} secret_get_data_t;

#ifndef __KERNEL__
static int fd_secret;

#define secret_init()                                 \
    do                                                \
    {                                                 \
        fd_secret = open("/proc/" PROC_SECRET, O_RDONLY); \
        if (fd_secret <= 0)                               \
            err(1, "secret_init");                    \
    } while (0)

static inline int secret_get_addr(unsigned long *va)
{
    return ioctl(fd_secret, IOCTL_SECRET_GET_ADDR, va);
}
static inline int secret_get_data(unsigned char *buffer, size_t size)
{
    secret_get_data_t args = {
        .buffer = buffer,
        .size = size,
    };
    return ioctl(fd_secret, IOCTL_SECRET_GET_DATA, &args);
}
#endif
