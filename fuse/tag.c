#include "./sys.h"
#include "./util.h"
#include "./tag.h"
#include "./file.h"
#include "./log.h"

#include "../cutil/hash_table.h"
#include "../cutil/list.h"
#include "../cutil/string2.h"

#define MAX_LENGTH 1000

static struct hash_table *tags;

INITIALIZER(tag_init)
{
    tags = ht_create(0, NULL);
}

static void tag_dec_ref(struct tag *t)
{
    -- t->ref_count;
    if (t->ref_count == 0) {
        ht_remove_entry(tags, t->value);
        free(t);
    }
}

static void tag_inc_ref(struct tag *t)
{
    ++ t->ref_count;
}

static struct tag *tag_new(const char *value, bool in_use)
{
    struct tag *t = calloc(sizeof*t, 1);
    t->value = strdup(value);
    t->files = ht_create(0, NULL);
    t->in_use = in_use;
    return t;
}

static struct tag* tag_get_or_create__(const char *value, bool in_use)
{
    struct tag *t;
    if (ht_get_entry(tags, value, &t) >= 0) {
        return t;
    }
    t = tag_new(value, in_use);

    ht_add_entry(tags, value, t);
    return t;
}

struct tag* tag_get_or_create(const char *value)
{
    struct tag *t;
    t = tag_get_or_create__(value, true);
    tag_inc_ref(t);
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
    ht_add_unique_entry(t->files, f->name, f);
    t->in_use = true;
}

void tag_remove_file(struct tag *t, struct file *f)
{
    ht_remove_entry(t->files, f->name);
}

void tag_remove(struct tag *t)
{
    struct hash_table *tmp = ht_create(0, NULL);
    void prepare_remove_tag(const char *name, void *f, void *ctx)
    {
        ht_add_entry(tmp, t->value, f);
    }
    ht_for_each(t->files, &prepare_remove_tag, t);

    void remove_tag(const char *name, void *f_, void *ctx)
    {
        struct file *f =  f_;
        file_remove_tag(f, t);
    }
    ht_for_each(tmp, &remove_tag, NULL);
    ht_free(tmp);

    ht_remove_entry(tags, t->value);
    tag_dec_ref(t);
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
        struct tag *t = tag_get(tags[i]);
        if (t == INVALID_TAG)
            t = tag_get_or_create__(tags[i], false);
        tag_inc_ref(t);
        ht_add_entry(selected_tags, tags[i], t);
        free(tags[i]);
    }
    free(tags);
    print_debug("selected tag: %d\n", i);
}

void free_selected_tags(struct hash_table *selected_tags)
{
    void free_tag(const char *n, void *t_, void *ctx)
    {
        struct tag *t = t_;
        if (!t->in_use)
            tag_remove(t);
    }
    ht_for_each(selected_tags, &free_tag, NULL);
    ht_free(selected_tags);
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
    print_debug("read tag list size = %d\n", s);
    for (int i = 1; i <= s; ++i) {
        struct file *f = list_get(l, i);

        if (ht_entry_count(f->tags) == 0)
            continue;

        fprintf(output, "[%s]\n", f->name);
        print_debug(_("print file %s\n"), f->name);
        void print_tag(const char *key, void *tag, void *value)
        {
            struct tag *t = tag;
            fprintf(output, "%s\n", t->value);
        }
        ht_for_each(f->tags, &print_tag, NULL);
        fprintf(output, "\n");
    }
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
                        print_debug(_("la structure tag n'existe pas\n"));
                    }
                }
            } else  {
                print_debug(_("la structure file n'existe pas\n"));
            }
        }
        fclose(lib);
    }
}
