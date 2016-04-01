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

static char *basenamedup(const char *dir)
{
    char *res, *tmp = strdup(dir);
    res = strdup(basename(tmp));
    free(tmp);
    return res;
}

static char *dirnamedup(const char *dir)
{
    char *res, *tmp = strdup(dir);
    res = strdup(dirname(tmp));
    free(tmp);
    return res;
}

static char *append_dir(const char *dir, const char *file)
{
    char *str;
    if (asprintf(&str, "%s/%s", dir, file) < 0) {
        ERROR("asprint allocation failed: %s\n", strerror(errno));
    }
    return str;
}

/*************************
 * File operations
 */

char *tag_realpath(const char *user_path)
{
    char *realpath, *file;
    /* remove directory names before the actual filename
     * so that grepped file '/foo/bar/toto' becomes 'toto'
     */
    file = basenamedup(user_path);

    /* prepend dirpath since the daemon runs in '/' */
    if (asprintf(&realpath, "%s/%s", realdirpath, file) < 0) {
        ERROR("asprintf: allocation failed: %s", strerror(errno));
    }
    free(file);
    return realpath;
}

static bool file_matches_tags(
    const char *filename, struct hash_table *selected_tags)
{
    struct file *f = file_get_or_create(filename);
    bool match = true;
    void check_tags(const char *name, void *value, void *arg) {
        bool *match = arg;
        struct tag *t = value;
        if (t == INVALID_TAG || !ht_has_entry(f->tags, t->value))
            *match = false;
    }
    DBG("selected tags size: %d\n", ht_entry_count(selected_tags));
    ht_for_each(selected_tags, &check_tags, &match);
    return match;
}

/* get attributes */
static int tag_getattr(const char *user_path, struct stat *stbuf)
{
    int res;
    char *realpath = tag_realpath(user_path);
    struct hash_table *selected_tags;
    
    LOG("getattr '%s'\n", user_path);

    char *dirpath, *filename;
    dirpath = dirnamedup(user_path);
    filename = basenamedup(user_path);
    compute_selected_tags(dirpath, &selected_tags);
    
    /* try to stat the actual file */
    if ((res = stat(realpath, stbuf)) < 0 || S_ISDIR(stbuf->st_mode)) {
        LOG("error ::%s :: %s\n", realpath, strerror(errno));
        /* if the file doesn't exist, check if it's a tag
           directory and stat the main directory instead */
        if (!strcmp(user_path, "/") ||
            (tag_exist(filename) &&
             !ht_has_entry(selected_tags, filename))) {
            res = stat(realdirpath, stbuf);
        } else {
            res = -ENOENT;
        }
    } else {
        /* if the file exist check that it have the selected tags*/
        if (ht_entry_count(selected_tags) > 0) {
            if (!file_matches_tags(filename, selected_tags)) {
                res = -ENOENT;
            }
        }
    }
    free(filename);
    free(dirpath);
    
    LOG("getattr returning '%s'\n", strerror(-res));
    free(realpath);
    return res;
}

static int tag_unlink(const char *user_path)
{
    return -EPERM;
}

static void readdir_list_files(
    void *buf, const char *user_path, fuse_fill_dir_t filler)
{
    struct dirent *dirent;
    
    rewinddir(realdir);
    while ((dirent = readdir(realdir)) != NULL) {
        struct stat stbuf;
        /* only list files, don't list directories */
        if (dirent->d_type == DT_DIR)
            continue;

        /* only files whose contents matches tags in path */
        char *virtpath = append_dir(user_path, dirent->d_name);
        int res = tag_getattr(virtpath, &stbuf);
        free(virtpath);
        if (res < 0)
            continue;
        filler(buf, dirent->d_name, NULL, 0);
    }
}

static void readdir_list_tags(
    void *buf, struct hash_table *selected_tags, fuse_fill_dir_t filler)
{
    // afficher les tags pas encore selectionné
    struct list *tagl = tag_list();
    unsigned s = list_size(tagl);
    DBG("tag list size: %u\n", s);

    for (int i = 1; i <= s; ++i) {
        struct tag *t = list_get(tagl, i);
        DBG("t->value = %s\n", t->value);
        DBG("selected size: %d\n", ht_entry_count(selected_tags));
        if (!ht_has_entry(selected_tags, t->value)) {
            DBG("on passe par la avec t->value = %s!\n", t->value);
            filler(buf, t->value, NULL, 0);
        }
    }
}

/* list files within directory */
static int tag_readdir(
    const char *user_path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
    struct hash_table *selected_tags;
    int res = 0;
    
    LOG("\n\nreaddir '%s'\n", user_path);
    
    compute_selected_tags(user_path, &selected_tags);
    readdir_list_tags(buf, selected_tags, filler);
    readdir_list_files(buf, user_path, filler);

    LOG("readdir returning %s\n\n\n", strerror(-res));
    
    return res;
}

/* read the content of the file */
static int tag_read(
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
    if (res >= 0 && res < len) {
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
        LOG("read returning '%s'\n", strerror(-res));
    else
        LOG("read returning success (read %d)\n", res);
    free(realpath);
    return res;
}

static int tag_write(
    const char *path, const char *buffer, size_t len,
    off_t off, struct fuse_file_info *fi)
{
    char *realpath = tag_realpath(path);
    int res;

    LOG("write '%s' for %ld bytes starting at offset %ld\n",
        path, len, off);

    int fd = open(realpath, O_WRONLY);
    if (fd < 0) {
        res = -errno;
        goto out_with_fd;
    }
    if (lseek(fd, off, SEEK_SET) < 0) {
        res = -errno;
        goto out_with_fd;
    }
    res = write(fd, buffer, len);
    if (res < 0) {
        res = -errno;
    }
  out_with_fd:
    close(fd);
  out:
    if (res < 0)
        LOG("write returning '%s'\n", strerror(-res));
    else
        LOG("write returning success (wrote %d)\n", res);
    free(realpath);
    return res;
}

static int tag_mknod(const char *user_path, mode_t mode, dev_t device)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = mknod(realpath, mode, device);
    free(realpath);
    return res;
}

static int tag_truncate(const char *user_path, off_t length)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = truncate(realpath, length);
    free(realpath);
    return res;
}

static int tag_chmod(const char *user_path, mode_t mode)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = chmod(realpath, mode);
    free(realpath);
    return res;
}

static int tag_chown(const char *user_path, uid_t user, gid_t group)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = chown(realpath, user, group);
    free(realpath);
    return res;
}

static int tag_utime(const char *user_path, struct utimbuf *times)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = utime(realpath, times);
    free(realpath);
    return res;
}

struct fuse_operations tag_oper = {
    .getattr = tag_getattr,
    .readdir = tag_readdir,

    .read = tag_read,
    .write = tag_write,

    .unlink = tag_unlink,
    .mknod = tag_mknod,
    .truncate = tag_truncate,
    .chmod = tag_chmod,
    .chown = tag_chown,
    .utime = tag_utime,
};

/**************************
 * Main
 */

