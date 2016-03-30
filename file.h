#ifndef FILE_H
#define FILE_H

struct file;

#include "cutil/hash_table.h"

struct file {
    char *name;
    struct hash_table *tags;
};



#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct file* file_get_or_create(const char *value);
struct file* file_get(const char *value);

#endif //FILE_H
