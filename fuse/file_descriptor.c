#include "./sys.h"
#include "./file_descriptor.h"

struct file_descriptor *fd_open(struct path *path, int flags, const bool is_tag)
{
    int fh = -1;
    if (!is_tag) {
        fh = open(path->realpath, flags);
        if (fh < 0) {
            print_debug("lulz %s\n", strerror(errno));
            return ERR_PTR(-errno);
        }
    }
    
    struct tag *t = INVALID_TAG;
    if (is_tag) {
        if (strcmp(path->virtpath, "/") != 0) {
            t = tag_get(path->filename);
            if (t == INVALID_TAG)
                return ERR_PTR(-ENOENT);
        }
    }
    
    struct file_descriptor *fd = calloc(sizeof*fd, 1);
    if (!fd)
        return ERR_PTR(-ENOMEM);

    if (!strcmp(path->filename, TAG_FILENAME))
        fd->is_tag_file = true;

    if (is_tag)
        fd->tag = t;
    else
        fd->file = file_get(path->filename);

    fd->is_tag = is_tag;
    fd->path = path;
    fd->fh = fh;
    return fd;
}

int fd_close(struct file_descriptor *fd)
{
    int err = 0;
    if (close(fd->fh) < 0)
        err = -errno;

    path_delete(fd->path);
    free(fd);
    return err;
}

int fd_read(struct file_descriptor *fd, char *buffer, size_t len, off_t off)
{
    int res = 0;
    if (lseek(fd->fh, off, SEEK_SET) < 0) {
        return -errno;
    }
    res = read(fd->fh, buffer, len);
    if (res >= 0 && res < len) {
        for (int i = res; i < len; ++i) {
            buffer[i] = 0;
        }
    }
    if (res < 0)
        res = -errno;

    return res;
}

int fd_write(struct file_descriptor *fd, const char *buffer, size_t len, off_t off)
{
    int res = 0;
    if (lseek(fd->fh, off, SEEK_SET) < 0) {
        return -errno;
    }
    res = write(fd->fh, buffer, len);
    if (res < 0)
        res = -errno;
    return res;
}
