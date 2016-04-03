#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static struct file *file_new(const char *name)
{
    struct file *t = calloc(sizeof*t, 1);
    t->name = strdup(name);
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

