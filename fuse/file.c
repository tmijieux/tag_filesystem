#include "./sys.h"
#include "./util.h"
#include "./path.h"
#include "./file.h"
#include "./tag.h"

static struct hash_table *files;

INITIALIZER(file_init)
{
    files = ht_create(0, NULL);
}

void file_add_tag(struct file *f, struct tag *t)
{
    ht_add_unique_entry(f->tags, t->value, t);
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
    struct file *t = malloc(sizeof*t);
    t->name = strdup(name);
    t->realpath = path_realpath(name);
    t->tags = ht_create(0, NULL);
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
