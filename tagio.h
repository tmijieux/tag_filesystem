#ifndef TAGIO_H
#define TAGIO_H

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

enum {
    TAGIOC_READ_TAGS   = _IO('E', 0),
};

struct tag_ioctl_rw {
    char            *buf;
    size_t          size;
    size_t          total_size;       /* out param for total size */
};

#endif //TAGIO_H
