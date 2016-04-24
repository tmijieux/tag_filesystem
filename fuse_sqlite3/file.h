#ifndef FILE_H
#define FILE_H

struct file;

#include "util.h"

struct file {
    int id;
    char *name;
    char *realpath;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "tag.h"

struct file* file_get_or_create(const char *value);
struct file* file_get(const char *value);
char *file_get_tags_string(const struct file *f, int *size);
struct list *file_list(void);

#endif //FILE_H
