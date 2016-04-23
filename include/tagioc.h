#ifndef TAGIO_H
#define TAGIO_H

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

#define BUFSIZE 1024

struct tag_ioctl {
    size_t size;
    off_t off;
    char buf[BUFSIZE];
};

enum {
    TAG_IOC_READ_TAGS = _IOWR('E', 0, struct tag_ioctl),
};

#endif //TAGIO_H
