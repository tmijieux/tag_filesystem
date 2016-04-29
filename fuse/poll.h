#ifndef POLL_H
#define POLL_H

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <poll.h>

struct poll_h {
    struct fuse_pollhandle *ph;
    unsigned *reventsp;
};

struct poll_h *poll_h_create(struct fuse_pollhandle *ph, unsigned *reventsp);
void poll_h_free(struct poll_h *p);

#endif //POLL_H
