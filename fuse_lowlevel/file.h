#ifndef FILE_H
#define FILE_H

struct file;

#include "./real.h"
#include "./util.h"
#include "./fuse_callback.h"

struct tnode {
    fuse_ino_t number;
    struct inode *inode;
    UT_hash_handle hh;
};

struct file {
    char *name;
    char *realpath;

    struct hash_table *tags;
//    struct tnode *tags;

    struct inode *ino;
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "tag.h"

struct file* file_get_or_create(const char *value);
struct file* file_get(const char *value);
struct list *file_list(void);

char *file_get_tags_string(const struct file *f, int *size);

#endif //FILE_H
