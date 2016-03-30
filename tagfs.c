/*
 * Copyright (c) 2014-2015 Inria. All rights reserved.
 */

#define _GNU_SOURCE
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <dirent.h>
#include <libgen.h>

#include "tag.h"
#include "log.h"
#include "cutil/string2.h"
#include "cutil/hash_table.h"
#include "cutil/list.h"

DIR *realdir;
char *realdirpath;

/*************************
 * File operations
 */

char *tag_realpath(const char *user_path)
{
    char *realpath, *path, *file;
    /* remove directory names before the actual filename
     * so that grepped file '/foo/bar/toto' becomes 'toto'
     */
    path = strdup(user_path);
    file = basename(path);
    
    /* prepend dirpath since the daemon runs in '/' */
    if (asprintf(&realpath, "%s/%s", realdirpath, file) < 0) {
        ERROR("asprintf: allocation failed: %s", strerror(errno));
    }
    free(path);
    return realpath;
}

/* get attributes */
static int tag_getattr(const char *user_path, struct stat *stbuf)
{
    char *realpath = tag_realpath(user_path);
    int res;

    LOG("\ngetattr '%s'\n", user_path);
    struct hash_table *selected_tags;
    char *path = strdup(user_path);
    char *dirpath = dirname(path);
    compute_selected_tags(dirpath, &selected_tags);
    
    /* try to stat the actual file */
    if ((res = stat(realpath, stbuf)) < 0) {
        LOG("error ::%s :: %s\n", realpath, strerror(errno));
        /* if the file doesn't exist, check if it's a tag
           directory and stat the main directory instead */
        char *path = strdup(user_path);
        char *filename = basename(path);
        
        if (tag_exist(filename) && !ht_has_entry(selected_tags, filename)) {
            res = stat(realdirpath, stbuf);
        } else {
            res = -ENOENT;
        }
    } else {
        if (ht_entry_count(selected_tags) > 0) {
            struct file *f = file_get_or_create(realpath);
            bool ok = true;
            void check_tags(const char *name, void *value, void *arg) {
                bool *ok = arg;
                struct tag *t = value;
                if (!ht_has_entry(f->tags, t->value))
                *ok = false;
            }
            ht_for_each(selected_tags, &check_tags, &ok);
            if (!ok)
                res = -ENOENT;
        }
    }

    LOG("getattr returning %s\n\n", strerror(-res));
    free(realpath);
    return res;
}

/* list files within directory */
static int tag_readdir(
    const char *user_path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
    struct dirent *dirent;
    int res = 0;
    struct hash_table *selected_tags;

    LOG("\n\nreaddir '%s'\n", user_path);
    compute_selected_tags(user_path, &selected_tags);
    
    rewinddir(realdir);
    while ((dirent = readdir(realdir)) != NULL) {
        struct stat stbuf;
        /* only files whose contents matches tags in path */
        res = tag_getattr(dirent->d_name, &stbuf);
        if (res < 0)
            continue;

        /* only list files, don't list directories */
        if (dirent->d_type == DT_DIR)
            continue;
        filler(buf, dirent->d_name, NULL, 0);
    }

    // afficher les tags pas encore selectionné

    struct list *tagl = tag_list();
    unsigned s = list_size(tagl);
    DEBUGMSG("tag list size: %u\n", s);
    
    for (int i = 1; i <= s; ++i) {
        struct tag *t = list_get(tagl, i);
        if (!ht_has_entry(selected_tags, t->value)) {
            filler(buf, t->value, NULL, 0);
        }
    }
    LOG("readdir returning %s\n\n\n", strerror(-res));
    return 0;
}

/* read the content of the file */
int tag_read(
    const char *path, char *buffer, size_t len,
    off_t off, struct fuse_file_info *fi)
{
    char *realpath = tag_realpath(path);
    int res;

    LOG("read '%s' for %ld bytes starting at offset %ld\n", path, len, off);

    int fd = open(realpath, O_RDONLY);
    if (fd < 0) {
        res = -errno;
        goto out_with_fd;
    }
    if (lseek(fd, off, SEEK_SET) < 0) {
        res = -errno;
        goto out_with_fd;
    }
    res = read(fd, buffer, len);
    if (res < len) {
        for (int i = res; i < len; ++i) {
            buffer[i] = 0;
        }
    }
    if (res < 0) {
        res = -errno;
    }
  out_with_fd:
    close(fd);
  out:
    if (res < 0)
        LOG("read returning %s\n", strerror(-res));
    else
        LOG("read returning success (read %d)\n", res);
    free(realpath);
    return res;
}

struct fuse_operations tag_oper = {
    .getattr = tag_getattr,
    .readdir = tag_readdir,
    .read = tag_read,
};

/**************************
 * Main
 */

