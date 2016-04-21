#include <assert.h>

#include "./inode.h"
#include "./real.h"

struct inode *itable = NULL;

static int next_inode_number(void)
{
    static fuse_ino_t next_ino = 1;
    return next_ino++;
}

static struct inode *inode_new(int directory)
{
    struct inode *ino = calloc(sizeof*ino, 1);
    ino->number = next_inode_number();
    ino->is_directory = directory;
    ino->is_tagfile = 0;
    ino->nlookup = 0;
    if (directory)
        ino->selected_tags = ht_create(0, NULL);

    HASH_ADD(hi, itable, number, sizeof(ino->number), ino);
    return ino;
}

void inode_init(void)
{
    struct inode *ino;
    // create the root directory with inode number 1;
    ino = inode_new(1);
    print_debug("ino->number = %d\n", ino->number);
    assert( ino->number == 1 );
    ino->nlookup = 1;
}

void inode_delete_if_noref(struct inode *ino)
{
    if (ino->number != 1 && ino->nlookup == 0) {
        print_debug("deleting inode = %lu\n", ino->number);
        HASH_DELETE(hi, itable, ino);
        if (!ino->is_directory) {
            free(ino->realpath);
        } else {
            for (int i = 0; i < ino->ntag; ++i)
                free(ino->tag[i]);
            free(ino->tag);
        }
        free(ino);
    }
}

struct inode *create_directory_inode(struct inode *parent, const char *name)
{
    int i;
    struct inode *ino = inode_new(1);
    ino->ntag = parent->ntag + 1;
    ino->tag = malloc(ino->ntag * sizeof*ino->tag);
    for (i = 0; i < parent->ntag; ++i) {
        char *tag = parent->tag[i];
        ino->tag[i] = strdup(tag);
        ht_add_entry(ino->selected_tags, tag, tag_get(tag));
    }
    ino->tag[i] = strdup(name);
    ino->name = ino->tag[i];
    ht_add_entry(ino->selected_tags, name, tag_get(name));
    return ino;
}

struct inode *get_or_create_inode(
    fuse_ino_t parent, const char *name, int directory)
{
    struct inode *parent_inode;

    HASH_FIND(hi, itable, &parent, sizeof(parent), parent_inode);
    if (parent_inode != NULL) {
        if (!parent_inode->is_directory) {
            print_debug("cp 1\n");
            return NULL;
        }

        struct inode *ino;
        ino = inode_new(directory);
        ino->name = strdup(name);
        return ino;
    }
    print_debug("cp 4\n");
    return NULL;
}

struct inode *inode_get(fuse_ino_t number)
{
    struct inode *ino;
    HASH_FIND(hi, itable, &number, sizeof(number), ino);
    return ino;
}

struct inode *get_or_create_file_inode(
    fuse_ino_t parent, const char *name)
{
    struct inode *ino;
    ino = get_or_create_inode(parent, name, 0);
    ino->realpath = tag_realpath(name);
    return ino;
}

void inode_set_statbuf(struct inode *ino, struct stat *st)
{
    ino->st = *st;
    ino->st.st_ino = ino->number;
}

void inode_get_statbuf(struct inode *ino, struct stat *st)
{
    *st = ino->st;
}
