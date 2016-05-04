#include "fuse.h"
#include "sys.h"
#include "util.h"
#include "path.h"
#include "file.h"
#include "file_descriptor.h"
#include "tag.h"

static struct hash_table *files;

INITIALIZER(file_init)
{
    files = ht_create(0, NULL);
}

static void notify_descriptors_poll(struct file *f, unsigned event)
{
    struct desc_table *entry, *tmp;
    HASH_ITER(hh, f->descriptors, entry, tmp) {
        struct fuse_pollhandle *ph = entry->fd->ph;
        if (ph != NULL) {
            f->revents |= event;
            fuse_notify_poll(ph);
        }
        print_debug("notifying tag/untag file %s\n", f->name);
    }
}

void file_add_tag(struct file *f, struct tag *t)
{
    ht_add_unique_entry(f->tags, t->value, t);
    notify_descriptors_poll(f, POLLIN);
}

void file_remove_tag(struct file *f, struct tag *t)
{
    ht_remove_entry(f->tags, t->value);
    notify_descriptors_poll(f, POLLOUT);
}

struct list *file_list(void)
{
    return ht_to_list(files);
}

static struct file *file_new(const char *name)
{
    struct file *t = malloc(sizeof*t);
    t->name = strdup(name);
    t->realpath = path_realpath(name);
    t->tags = ht_create(0, NULL);
    t->descriptors = NULL;
    t->revents = 0;
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

char *file_get_tags_string(const struct file *f, int *size)
{
    int c = 0;
    char *str = strdup("");
    *size = 0;
    void each_tag(const char *n, void *tag, void *arg)
    {
        char *tmp;
        struct tag *t = tag;
        tmp = str;
        *size = asprintf(&str, "%s%s%s", str, 0==c?"":" ",t->value);
        free(tmp);
        ++c;
    }
    ht_for_each(f->tags, each_tag, NULL);
    return str;
}

void file_add_descriptor(struct file *f, struct file_descriptor *fd)
{
    struct desc_table *dt = calloc(sizeof*dt, 1);
    dt->fd = fd;
    HASH_ADD_PTR(f->descriptors, fd, dt);
}

void file_remove_descriptor(struct file *f, struct file_descriptor *fd)
{
    struct desc_table *entry = NULL;
    HASH_FIND_PTR(f->descriptors, fd, entry);
    if (entry != NULL) {
        HASH_DEL(f->descriptors, entry);
        free(entry);
    }
}
