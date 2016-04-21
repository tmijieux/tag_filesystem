#include <unistd.h>

#include "./file_descriptor.h"

int file_open(struct file *f, int flags)
{
    return open(f->realpath, flags);
}

int file_read(int fd, char *buffer, size_t len, off_t off)
{
    int res = 0;
    if (lseek(fd, off, SEEK_SET) < 0) {
        return -errno;
    }
    res = read(fd, buffer, len);
    if (res >= 0 && res < len) {
        for (int i = res; i < len; ++i) {
            buffer[i] = 0;
        }
    }
    if (res < 0)
        res = -errno;

    return res;
}

int file_write(int fd, const char *buffer, size_t len, off_t off)
{
    int res = 0;
    if (lseek(fd, off, SEEK_SET) < 0) {
        return -errno;
     }
    res = write(fd, buffer, len);
    if (res < 0)
        res = -errno;
    return res;
}

int file_close(int fd)
{
    int r = close(fd);
    if (r < 0)
        r = -errno;
    return r;
}
