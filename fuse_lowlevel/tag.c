#include <stdbool.h>
#include <libgen.h>
#include <string.h>
#include "tag.h"
#include "file.h"
#include "log.h"

#include "cutil/hash_table.h"
#include "cutil/list.h"
#include "cutil/string2.h"
#include "./inode.h"

static struct hash_table *tags;

__attribute__((constructor))
static void tag_init(void)
{
    tags = ht_create(0, NULL);
}

static struct tag *tag_new(const char *value)
{
    struct tag *t = calloc(sizeof*t, 1);
    t->value = strdup(value);
    t->files = ht_create(0, NULL);
    return t;
}

struct tag* tag_get_or_create(const char *value)
{
    struct tag *t;
    if (ht_get_entry(tags, value, &t) >= 0) {
        return t;
    }
    t = tag_new(value);

    ht_add_entry(tags, value, t);
    return t;
}

struct tag* tag_get(const char *value)
{
    struct tag *t;
    if (ht_get_entry(tags, value, &t) >= 0) {
        return t;
    }
    return INVALID_TAG;
}

bool tag_exist(const char *value)
{
    return (INVALID_TAG != tag_get(value));
}

static bool tag_complete_match(struct hash_table *tags, char **tag, int n)
{
    for (int i = 0; i < n; ++i) {
        if (!ht_has_entry(tags, tag[i]))
            return false;
    }
    return true;
}

void tag_remove(struct tag *t)
{
    void remove_tag(const char *name, void *f, void *t)
    {
        ht_remove_entry(((struct file*)f)->tags, ((struct tag*)t)->value);
    }
    ht_for_each(t->files, &remove_tag, t);
    ht_remove_entry(tags, t->value);
    free(t);
}

void compute_selected_tags(
    const char *dirpath, struct hash_table **ret)
{
    int i;

    struct hash_table *selected_tags;
    *ret = selected_tags = ht_create(0, NULL);

    if (!strcmp(dirpath, "."))
        return ;

    char **tags ;
    int tag_count = string_split(dirpath, "/", &tags);

    for (i = 0; i < tag_count; ++i) {
        DBG("selected tag: %s\n", tags[i]);
        ht_add_entry(selected_tags, tags[i], tag_get(tags[i]));
        free(tags[i]);
    }
    free(tags);
    DBG("selected tag: %d\n", i);
}

struct list *tag_list(void)
{
    return ht_to_list(tags);
}

void tag_file(struct tag *t, struct file *f)
{
    ht_add_unique_entry(t->files, f->name, f);
    ht_add_unique_entry(f->tags, t->value, t);
}

void untag_file(struct tag *t, struct file *f)
{
    ht_remove_entry(f->tags, t->value);
    ht_remove_entry(t->files, f->name);
}

void tag_db_dump(FILE *output)
{
    struct list *l = file_list();
    int s = list_size(l);
    DBG("read tag list size = %d\n", s);
    for (int i = 1; i <= s; ++i) {
        struct file *f = list_get(l, i);

         if (ht_entry_count(f->tags) == 0)
             continue;

        fprintf(output, "[%s]\n", f->name);
        DBG("print file %s\n", f->name);
        void print_tag(const char *key, void *tag, void *value)
        {
            struct tag *t = tag;
            fprintf(output, "%s\n", t->value);
        }
        ht_for_each(f->tags, &print_tag, NULL);
        fprintf(output, "\n");
    }
}

void update_lib(char *tagFile)
{
    struct list *filesList = file_list();
    FILE *lib = NULL;
    lib = fopen(tagFile, "w+");

    if (lib != NULL) {
        int s1 = list_size(filesList);
        for (int i = 1; i <= s1; i++) {
            struct file *fi = list_get(filesList, i);

            if (fi != NULL) {
                struct hash_table *fiTags = fi->tags;
                struct list *fiTagsList = ht_to_list(fiTags);

                fprintf(lib, "[%s]\n", fi->name);
                int s2 = list_size(fiTagsList);
                for (int j = 1; j <= s2; ++j) {
                    struct tag *t = list_get(fiTagsList, j);
                    if (t != NULL) {
                        fprintf(lib, "%s\n", t->value);
                    } else {
                        print_debug("la structure tag n'existe pas\n");
                    }
                }
            } else  {
                print_debug("la structure file n'existe pas\n");
            }
        }
        fclose(lib);
    }
}
