#ifndef TAG_H
#define TAG_H

struct tag {
    char *value;
    struct list *file_list;
};

#include <stdbool.h>
#include "file.h"
#include "cutil/hash_table.h"

struct tag* tag_get_or_create(const char *value);
struct tag* tag_get(const char *value);
bool tag_exist(const char *value);
void tag_add_file(struct tag *t, struct file *f);
void compute_selected_tags(const char *user_path, struct hash_table **ret);
struct list *tag_list(void);
    
#endif //TAG_H
