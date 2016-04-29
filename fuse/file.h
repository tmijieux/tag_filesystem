#ifndef FILE_H
#define FILE_H

struct file;
struct desc_table;

#include "util.h"
#include "file_descriptor.h"

struct desc_table {
    struct file_descriptor *fd;
    UT_hash_handle hh;
};

struct file {
    char *name;
    char *realpath;
    struct hash_table *tags;
    struct desc_table *descriptors;
    unsigned revents;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "tag.h"

struct file* file_get_or_create(const char *value);
struct file* file_get(const char *value);
void file_add_tag(struct file *f, struct tag *t);
void file_remove_tag(struct file *f, struct tag *t);
char *file_get_tags_string(const struct file *f, int *size);
struct list *file_list(void);

void file_add_descriptor(struct file *f, struct file_descriptor *fd);
void file_remove_descriptor(struct file *f, struct file_descriptor *fd);

#endif //FILE_H
