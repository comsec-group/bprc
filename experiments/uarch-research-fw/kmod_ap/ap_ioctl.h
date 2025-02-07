// SPDX-License-Identifier: GPL-3.0-only
#define AP_IOCTL_RUN		0x10000 /* an arbitrarily picked request id */

#define PROC_AP "ap"

struct ap_payload {
    void (*fptr)(void *);
    void *data;
};

#ifndef __KERNEL__
#include <sys/ioctl.h>
#include <fcntl.h>
#include <err.h>
static int fd_ap;
#define ap_init() do { \
    fd_ap = open("/proc/" PROC_AP, O_RDONLY); \
    if (fd_ap <= 0) err(1, "ap_init");\
} while(0)

static inline int ap_run(void (*fptr)(void *), void *data) {
    struct ap_payload p;
    p.fptr = fptr;
    p.data = data;
    return ioctl(fd_ap, AP_IOCTL_RUN, &p);
}
#endif
