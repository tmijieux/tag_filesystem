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
    struct file *file;
    bool is_directory;
    bool is_tagfile;
    int fd;

    struct hash_table *selected_tags;
    const char *virtpath;
    const char *virtdirpath;

    const char *realpath;
};

char *tag_realpath(const char *user_path);

int filedes_create(const char *user_path, int flags, struct filedes **fd__);
int filedes_delete(struct filedes *fd);

#endif //FILEDES_H
