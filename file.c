
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct hash_table *files;


__attribute__((constructor))
static void file_init(void)
{
    files = ht_create(0, NULL);
}

static struct file *file_new(const char *name)
{
    struct file *t = malloc(sizeof*t);
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
