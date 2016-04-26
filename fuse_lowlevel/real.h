#ifndef REAL_H
#define REAL_H

struct file_descriptor;

#include "../cutil/hash_table.h"
#include "file.h"
#include <stdbool.h>
#include <dirent.h>

extern DIR *realdir;
extern char *realdirpath;
extern char *tagdbpath;

char *tag_realpath(const char *user_path);
void realdir_set_root(const char *path);
void realdir_add_files(void);
void realdir_parse_tags(void);

#endif //REAL_H
