#ifndef FILE_H
#define FILE_H

struct file;
struct file_descriptor;

#include "cutil/hash_table.h"
#include <stdbool.h>

struct filedes {
    struct file *file;
    bool is_directory;
    int fd;

    struct hash_table *selected_tags;
    const char *virtpath;
    const char *virtdirpath;

    const char *realpath;
};


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
    
#endif //FILE_H
