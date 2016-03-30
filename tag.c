
#include <stdbool.h>
#include <libgen.h>
#include <string.h>
#include "tag.h"
#include "file.h"
#include "log.h"

#include "cutil/hash_table.h"
#include "cutil/list.h"
#include "cutil/string2.h"

#define SENTINEL ((void*) 0XDEADBEEFCAFEBABE)

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
    struct tag *t = malloc(sizeof*t);
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
    return NULL;
}

bool tag_exist(const char *value)
{
    return (NULL != tag_get(value));
}

void tag_add_file(struct tag *t, struct file *f)
{
    list_add(t->file_list, f);
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
        ht_add_entry(selected_tags, tags[i], tag_get(tags[i]));
        DEBUGMSG("selected tag: %s\n", tags[i]);
        free(tags[i]);
    }
    free(tags);
    DEBUGMSG("selected tag: %d\n", i);
}

struct list *tag_list(void)
{
    return ht_to_list(tags);
}
