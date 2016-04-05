#include "sys.h"
#include "util.h"
#include "file.h"
#include "tag.h"

static struct hash_table *files;

__attribute__((constructor))
static void file_init(void)
{
    files = ht_create(0, NULL);
}

void file_add_tag(struct file *f, struct tag *t)
{
    ht_add_entry(f->tags, t->value, t);
}

void file_remove_tag(struct file *f, struct tag *t)
{
    ht_remove_entry(f->tags, t->value);
}

struct list *file_list(void)
{
    return ht_to_list(files);
}

static struct file *file_new(const char *name)
{
    struct file *t = calloc(sizeof*t, 1);
    t->name = strdup(name);
    t->tags = ht_create(0, NULL);
    t->fd = -1;
    t->opened = false;
    return t;
}

struct file* file_get_or_create(const char *value)
{
    struct file *t;
    if (ht_get_entry(files, value, &t) >= 0) {
        return t;
    }
    t = file_new(value);

    ht_add_entry(files, value, t);
    return t;
}

struct file* file_get(const char *value)
{
    struct file *t;
    if (ht_get_entry(files, value, &t) >= 0) {
        return t;
    }
    return NULL;
}

int file_open(struct file *f, int flags)
{
    int res = -1;
    if (!f->opened) {
        res = f->fd = open(f->realpath, flags);
        f->opened = true;
    }
    return res;
}


int file_close(struct file *f)
{
    int ret = -1;
    if (f->opened) {
        ret = close(f->fd);
        f->opened = false;
        f->fd = -1;
    }
    return ret;
}

int file_read(struct file *f, char *buffer, size_t len, off_t off)
{
    int res = 0;
    if (lseek(f->fd, off, SEEK_SET) < 0) {
        return -errno;
    }
    res = read(f->fd, buffer, len);
    if (res >= 0 && res < len) {
        for (int i = res; i < len; ++i) {
            buffer[i] = 0;
        }
    }
    if (res < 0)
        res = -errno;

    return res;
}

int file_write(struct file *f, const char *buffer, size_t len, off_t off)
{
    int res = 0;
    if (lseek(f->fd, off, SEEK_SET) < 0) {
        return -errno;
     }
    res = write(f->fd, buffer, len);
    if (res < 0)
        res = -errno;
    return res;
}
