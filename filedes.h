#ifndef FILEDES_H
#define FILEDES_H

struct file_descriptor;

#include "cutil/hash_table.h"
#include "file.h"
#include <stdbool.h>
#include <dirent.h>

extern DIR *realdir;
extern char *realdirpath;

struct filedes {
    int flags;
    const char *realpath;

    const char *virtpath;
    const char *virtdirpath;
    const char *filename;

    bool is_directory;
    bool is_tagfile;

    struct hash_table *selected_tags;
};

char *tag_realpath(const char *user_path);

struct filedes *filedes_create(const char *user_path);
void filedes_delete(struct filedes *fd);

#endif //FILEDES_H
