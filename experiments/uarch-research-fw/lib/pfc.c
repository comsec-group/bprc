// SPDX-License-Identifier: GPL-3.0-only
#include "pfc.h"

void pfc_dispose(struct pfc *p)
{
    munmap(p->buf, 0x1000);
    close(p->fd);
    p->fd = 0;
    p->buf = 0;
}

void alloc_measure_ctxt(measure_ctxt_t *ctxt, size_t measurement_rounds)
{
    memset((ctxt), 0, sizeof(*(ctxt)));
    (ctxt)->starts = calloc(measurement_rounds, sizeof((ctxt)->starts[0]));
    (ctxt)->ends = calloc(measurement_rounds, sizeof((ctxt)->ends[0]));
    if (!(ctxt)->starts || !(ctxt)->ends)
    {
        err(1, "failed to alloc measurement memory");
    };
    (ctxt)->total_rounds = measurement_rounds;
}

int _pfc_init(struct pfc *p, int no_user, int no_kernel, int ecore_quirk)
{
    struct perf_event_attr pe; // we move this to kernel
    memset(&pe, 0, sizeof(pe));
    pe.config = p->config;
    pe.config1 = p->config1;
    pe.config2 = p->config2;
    pe.size = sizeof(pe);
    if (ecore_quirk)
    {
        pe.type = 10;
    }
    else
    {
        pe.type = PERF_TYPE_RAW;
    }
    pe.sample_type = PERF_SAMPLE_CPU | PERF_SAMPLE_RAW;
    pe.exclude_user = no_user;
    pe.exclude_kernel = no_kernel;
    pe.exclude_hv = 1;
    pe.exclude_guest = 1;
    pe.exclude_idle = 1;
    pe.exclude_callchain_kernel = 1;
    p->fd = perf_event_open(&pe, 0, -1, -1, PERF_FLAG_FD_NO_GROUP);
    if (p->fd < 0)
        return -1;

    p->buf = mmap(NULL, 0x1000, PROT_READ, MAP_SHARED, p->fd, 0);
    if (p->buf == MAP_FAILED)
    {
        close(p->fd);
        return -1;
    }
    return 0;
}
