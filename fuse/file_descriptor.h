#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include <stdbool.h>

struct file_descriptor;

#include "./path.h"
#include "err.h"

struct file_descriptor {
    struct path *path;
    bool is_tag;
    bool is_tag_file;

    struct fuse_pollhandle *ph;
    union {
        struct file *file;
        struct tag *tag;
    };
    int fh;
};

struct file_descriptor *fd_open(struct path *path, int flags, bool is_tag);
int fd_close(struct file_descriptor *fd);

int fd_read(struct file_descriptor *f, char *buffer, size_t len, off_t off);
int fd_write(struct file_descriptor *f, const char *buffer, size_t len, off_t off);

#endif //FILE_DESCRIPTOR_H

