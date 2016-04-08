#include <stdbool.h>
#include <libgen.h>
#include <string.h>
#include "tag.h"
#include "file.h"
#include "log.h"

#include "cutil/hash_table.h"
#include "cutil/list.h"
#include "cutil/string2.h"

static struct hash_table *tags;

__attribute__((constructor))
static void tag_init(void)
{
    tags = ht_create(0, NULL);

    #ifdef DEBUG
    tag_get_or_create("nuit");
    tag_get_or_create("paysage");
    #endif
}

static struct tag *tag_new(const char *value)
{
    struct tag *t = calloc(sizeof*t, 1);
    t->value = strdup(value);
    t->file_list = list_new(0);
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

void tag_add_file(struct tag *t, struct file *f)
{
    list_remove_value(t->file_list, f);
    list_add(t->file_list, f);
}

void tag_remove_file(struct tag *t, struct file *f)
{
    list_remove_value(t->file_list, f);
}

void tag_remove(struct tag *t)
{
    int s = list_size(t->file_list);
    for (int i = 1; i <= s; ++i) {
        struct file *f;

        f = list_get(t->file_list, i);
        file_remove_tag(f, t);
    }

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
    file_add_tag(f, t);
    tag_add_file(t, f);
}

void untag_file(struct tag *t, struct file *f)
{
    file_remove_tag(f, t);
    tag_remove_file(t, f);
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
