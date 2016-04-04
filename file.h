#ifndef FILE_H
#define FILE_H

struct file;

#include "cutil/hash_table.h"
#include "filedes.h"

struct file {
    char *name;
    struct hash_table *tags;
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
struct list *file_list(void);


#endif //FILE_H
