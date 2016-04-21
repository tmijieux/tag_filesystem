#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include "./file.h"

int file_open(struct file *f, int flags);
int file_close(int fd);
int file_read(int fd, char *buffer, size_t len, off_t off);
int file_write(int fd, const char *buffer, size_t len, off_t off);

#endif //FILE_DESCRIPTOR_H
