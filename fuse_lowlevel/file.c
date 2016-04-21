#include "./sys.h"
#include "./util.h"
#include "./file.h"
#include "./tag.h"
#include "./inode.h"

static struct hash_table *files;

__attribute__((constructor))
static void file_init(void)
{
    files = ht_create(0, NULL);
}

struct list *file_list(void)
{
    return ht_to_list(files);
}

static struct file *file_new(const char *name)
{
    struct file *f = calloc(sizeof*f, 1);
    char *tmp = append_dir(realdirpath, name);
    f->realpath = tag_realpath(tmp);
    free(tmp);
    f->name = strdup(name);
    f->tags = ht_create(0, NULL);
    f->ino = get_or_create_file_inode(1, name);
    if (!strcmp(name, TAG_FILENAME)) {
        f->ino->is_tagfile = 1;
    }
    print_debug("file: %s\n", name);
    return f;
}

struct file* file_get_or_create(const char *value)
{
    struct file *t;
    if (ht_get_entry(files, value, &t) >= 0) {
        return t;
    }
    t = file_new(value);

    ht_add_entry(files, value, t);
    return t;
}

struct file* file_get(const char *value)
{
    struct file *t;
    if (ht_get_entry(files, value, &t) >= 0) {
        return t;
    }
    return NULL;
}

char *file_get_tags_string(const struct file *f, int *size)
{
    int c = 0;
    char *str = strdup("");
    void each_tag(const char *n, void *tag, void *arg)
    {
        char *tmp;
        struct tag *t = tag;
        tmp = str;
        *size = asprintf(&str, "%s%s%s", str, 0==c?"":" ",t->value);
        free(tmp);
        ++c;
    }
    ht_for_each(f->tags, each_tag, NULL);
    return str;
}
