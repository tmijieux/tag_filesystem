#ifndef DB_H
#define DB_H

#define FUSE_USE_VERSION 26
#include <fuse.h>

int db_add_tag(const char *name);
int db_add_file(const char *name);
int db_list_all_tags(void *buf, fuse_fill_dir_t filler);
int db_list_remaining_tags(
    struct hash_table *selected_tags, void *buf, fuse_fill_dir_t filler);
int db_list_selected_files(
    struct hash_table *selected_tags, void *buf, fuse_fill_dir_t filler);
int db_tag_file(int t_id, int f_id);


#endif //DB_H
