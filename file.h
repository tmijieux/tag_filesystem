#ifndef FILE_H
#define FILE_H

struct file;

#include "filedes.h"
#include "util.h"

struct file {
    char *name;
    char *realpath;

    struct hash_table *tags;

    int fd;
    bool opened;
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

int file_open(struct file *f, int flags);
int file_close(struct file *f);
int file_read(struct file *f, char *buffer, size_t len, off_t off);
int file_write(struct file *f, const char *buffer, size_t len, off_t off);

#endif //FILE_H
