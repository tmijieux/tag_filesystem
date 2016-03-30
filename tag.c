
#include "tag.h"
#include "file.h"

#include "cutil/hash_table.h"
#include "cutil/list.h"

static struct hash_table *tags;

__attribute__((constructor))
static void tag_init(void)
{
    tags = ht_create(0, NULL);
    
    #ifdef NO_PARSE
    tag_get_or_create("nuit");
    tag_get_or_create("paysage");
    #endif
}

static struct tag *tag_new(char *value)
{
    struct tag *t = malloc(sizeof*t);
    t->value = value;
    t->file_list = list_new(0);
    return t;
}

struct tag* tag_get_or_create(char *value)
{
    struct tag *t;
    if (ht_get_entry(tags, value, &t) >= 0) {
        return t;
    } 
    t = tag_new(value);
    
    ht_add_entry(tags, value, t);
    return t;
}

struct tag* tag_get(char *value)
{
    struct tag *t;
    if (ht_get_entry(tags, value, &t) >= 0) {
        return t;
    } 
    return NULL;
}

void tag_add_file(struct tag *t, struct file *f)
{
    list_add(t->file_list, f);
}

