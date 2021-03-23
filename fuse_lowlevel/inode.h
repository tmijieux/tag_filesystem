#ifndef INODE_H
#define INODE_H

#include "./../cutil/uthash.h"
#include "./fuse_callback.h"

struct inode;
extern struct inode *itable;

struct inode {
    fuse_ino_t number;
    
    int is_tagfile;    
    int is_directory;

    struct stat st;
    uint64_t nlookup;
    
    struct hash_table *selected_tags;
    UT_hash_handle hi; // global table

    char *name; // file or tag name
    char *realpath;
    char **tag;
    int ntag;
};

void inode_init(void);

struct inode *get_or_create_file_inode(fuse_ino_t parent, const char *name);
struct inode *create_directory_inode(struct inode *parent, const char *name);
struct inode *inode_get(fuse_ino_t number);
void inode_delete_if_noref(struct inode *ino);

#endif //INODE_H
