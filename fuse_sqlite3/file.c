#include "./sys.h"
#include "./util.h"
#include "./path.h"
#include "./file.h"
#include "./tag.h"

static struct hash_table *files;

static int next_file_id(void)
{
    static int id = 0;
    return id++;
}

INITIALIZER(file_init)
{
    files = ht_create(0, NULL);
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
    t->id = next_file_id();
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
    char *str = strdup("");
    return str;
}
