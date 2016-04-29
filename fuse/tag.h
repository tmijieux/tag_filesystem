#ifndef TAG_H
#define TAG_H

struct tag {
    char *value;
    struct hash_table *files;
};

#define INVALID_TAG ((struct tag*)0xdeadcafebeefbabe)
#define TAG_FILENAME ".tags"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include "file.h"
#include "../cutil/hash_table.h"


struct tag* tag_get_or_create(const char *value);
struct tag* tag_get(const char *value);
bool tag_exist(const char *value);
void tag_add_file(struct tag *t, struct file *f);
void tag_remove(struct tag *t);
void compute_selected_tags(const char *user_path, struct hash_table **ret);
struct list *tag_list(void);
void tag_file(struct tag *t, struct file *f);
void untag_file(struct tag *t, struct file *f);
void parse_tags_db(const char *filename,
                   int (*getattr)(const char *, struct stat*));
void tag_db_dump(FILE *output);
void update_lib(char *tagFile);

#endif //TAG_H
