#ifndef FILEDES_H
#define FILEDES_H

#include <stdbool.h>
#include <dirent.h>

struct file_descriptor;

#include "./../cutil/hash_table.h"
#include "./file.h"

extern DIR *realdir;
extern char *realdirpath;
extern char *tagdbpath;

struct path {
    int depth;
    const char *realpath;
    const char *virtpath;
    const char *virtdirpath;
    const char *filename;
    struct hash_table *selected_tags;
};

char *path_realpath(const char *user_path);
struct path *path_create(const char *user_path, bool is_tag);
void path_delete(struct path *path);
void path_set(struct path *p, const char *virtdirpath, const char *filename,
              struct hash_table *selected_tags);
void path_free(struct path *path);


#endif //FILEDES_H
