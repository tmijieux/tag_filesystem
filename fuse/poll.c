#include <stdio.h>
#include <stdlib.h>
#include "poll.h"

struct poll_h *poll_h_create(struct fuse_pollhandle *ph, unsigned *reventsp)
{
    struct poll_h *p = calloc(sizeof *p, 1);
    p->ph = ph;
    p->reventsp = reventsp;
    return p;
}

void poll_h_free(struct poll_h *p)
{
    if (p != NULL) {
        fuse_pollhandle_destroy(p->ph);
        free(p);
    }
}
