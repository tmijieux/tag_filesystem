#ifndef TAGIO_H
#define TAGIO_H

#include <sys/types.h>
#include <sys/uio.h>
#include <sys/ioctl.h>

#define BUFSIZE 1024

struct tag_ioctl {
    int id;
    int again;
    int size;
    char buf[BUFSIZE];
};

enum {
    TAGIOC_READ_TAGS = _IOR('E', 0, struct tag_ioctl),
};

#endif //TAGIO_H
