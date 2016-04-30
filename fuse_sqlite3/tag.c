#include "./sys.h"
#include "./util.h"
#include "./tag.h"
#include "./file.h"
#include "./db.h"

#include "../cutil/hash_table.h"
#include "../cutil/list.h"
#include "../cutil/string2.h"

#define MAX_LENGTH 1000

static struct hash_table *tags;

static int next_tag_id(void)
{
    static int id = 0;
    return id++;
}

INITIALIZER(tag_init)
{
    tags = ht_create(0, NULL);
}

static struct tag *tag_new(const char *value)
{
    struct tag *t = calloc(sizeof*t, 1);
    t->value = strdup(value);
    t->id = next_tag_id();
    db_add_tag(t->value, t->id);
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

void tag_remove(struct tag *t)
{
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
        print_debug("selected tag: %s\n", tags[i]);
        ht_add_entry(selected_tags, tags[i], tag_get(tags[i]));
        free(tags[i]);
    }
    free(tags);
    print_debug("selected tag: %d\n", i);
}

struct list *tag_list(void)
{
    return ht_to_list(tags);
}

void tag_file(struct tag *t, struct file *f)
{
    db_tag_file(t->id, f->id);
}

void untag_file(struct tag *t, struct file *f)
{
    db_untag_file(t->id, f->id);
}

void tag_db_dump(FILE *output)
{

}

static char *copy_word_until(char *word, char end)
{
    return strndup(word, strchr(word, end) - word);
}

static struct file *parse_file(
    char *word, int(*getattr)(const char*,struct stat*))
{
    struct stat st;
    struct file *f = NULL;

    char *data = copy_word_until(word, ']');
    printf(_("file %s\n"), data);
    if (getattr(data, &st) >= 0)
        f = file_get_or_create(data);
    else
        print_error(_("file %s do not exist!\n"), data);
    free(data);
    return f;
}

static void parse_tag(struct file *f, char *word)
{
    char *data = copy_word_until(word, '\n');
    if (strlen(data)) {
        struct tag *t = tag_get_or_create(data);
        printf(_("tag %s\n"), data);
        if (f != NULL)
            tag_file(t, f);
    }
    free(data);
}

static void parse_tags_db2(FILE *fi, int (*getattr)(const char *, struct stat*))
{
    int i;
    struct file *f = NULL;
    char word[MAX_LENGTH] = "";

    while (fgets(word, MAX_LENGTH, fi) != NULL) {
        if (word[0] == '#')
            continue;
        for (i = 0; isspace(word[i]); ++i)
            ;
        if (word[i] == '[') {
            f = parse_file(word+i+1, getattr);
        } else {
            parse_tag(f, word+i);
        }
    }
}

void parse_tags_db(
    const char *filename, int (*getattr)(const char *, struct stat*))
{
    FILE *fi = NULL;
    fi = fopen(filename, "r+");
    if (fi != NULL) {
        parse_tags_db2(fi, getattr);
    }
    fclose(fi);
}
