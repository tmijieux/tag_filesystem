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
#include <limits.h>

#include "tag.h"
#include "log.h"
#include "cutil/string2.h"
#include "cutil/hash_table.h"
#include "cutil/list.h"


#define min(a, b) ((a) < (b) ? (a) : (b))
DIR *realdir;
char *realdirpath;

static int get_character_count(const char *str, char c)
{
    int n = 0;
    for (int i = 0; str[i]; ++i)
        if (str[i] == c)
            ++n;
    return n;
}

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
    /* viva el gcc: */
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

int tag_open(const char *user_path, struct fuse_file_info *fi)
{
    int res = 0;
    struct filedes *fd = calloc(sizeof*fd, 1);

    fd->realpath = tag_realpath(user_path);
    fd->fd = open(fd->realpath, fi->flags);
    if (fd->fd < 0) {
        res = -errno;
        free((char*)fd->realpath);
    } else {
        fd->virtpath = strdup(user_path);
        fd->virtdirpath = dirnamedup(user_path);
        fd->is_directory = false;
        fd->is_tagfile = false;
        char *name = basenamedup(user_path);
        if (!strcmp(name, ".tag"))
            fd->is_tagfile = true;
            
        fd->file = file_get_or_create(name);
        free(name);
        compute_selected_tags(fd->virtdirpath, &fd->selected_tags);
    }

    fi->fh = (uint64_t) fd;

    return res;
}

int tag_release(const char *user_path, struct fuse_file_info *fi)
{
    struct filedes *fd = (struct filedes*) fi->fh;
    free((char*) fd->realpath);
    free((char*) fd->virtpath);
    free((char*) fd->virtdirpath);
    ht_free(fd->selected_tags);
    int res = close(fd->fd);
    free(fd);
    return res;
}

static int getattr_intra(
    const char *user_path, struct stat *stbuf,
    struct hash_table *selected_tags,
    const char *realpath, const char *filename)
{
    int res;

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
            } else {
                /* let our file appear as a symlink */
                stbuf->st_mode &= ~S_IFMT;
                stbuf->st_mode |= S_IFLNK;
            }
        } else if (!strcmp(filename, ".tag")) {
            /* tag file is special and have "almost infinite" size */
            stbuf->st_size = (off_t) LONG_MAX;
        }
    }
    return res;
}

int tag_readlink(const char *user_path, char *buf, size_t size)
{
    int slash_count;
    char *to, *virtdirpath;
    
    to = basenamedup(user_path);
    virtdirpath = dirnamedup(user_path);
    slash_count = get_character_count(virtdirpath, '/');

    for (int i = 0; i < slash_count; ++i) {
        char *tmp = to;
        to = append_dir("..", tmp);
        free(tmp);
    }

    strncpy(buf, to, size);
    free(virtdirpath);
    free(to);
    return 0;
}

/* get attributes */
int tag_getattr(const char *user_path, struct stat *stbuf)
{
    int res;
    struct hash_table *selected_tags;
    char *dirpath, *filename, *realpath;
    
    LOG("getattr '%s'\n", user_path);
    
    realpath = tag_realpath(user_path);
    filename = basenamedup(user_path);
    dirpath = dirnamedup(user_path);
    compute_selected_tags(dirpath, &selected_tags);

    res = getattr_intra(
        user_path, stbuf, selected_tags, realpath, filename);

    LOG("getattr returning '%s'\n", strerror(-res));

    free(filename);
    free(realpath);
    free(dirpath);
    
    return res;
}

int tag_fgetattr(
    const char *user_path, struct stat *stbuf, struct fuse_file_info *fi)
{
    struct filedes *fd = (struct filedes*)fi->fh;
    return getattr_intra(
        user_path, stbuf, fd->selected_tags, fd->realpath, fd->file->name);
}

int tag_unlink(const char *user_path)
{
    int res = 0;
    int slash_count = get_character_count(user_path, '/');
        
    LOG("unlink '%s'\n", user_path);
    if (slash_count <= 1) {
        res =  -EPERM;
        goto end;
    }
    struct file *f;    
    char *dirpath, *filename;
    struct hash_table *selected_tags;
    
    filename = basenamedup(user_path);
    f = file_get(filename);
    if (NULL == f) {
        res = -ENOENT;
        free(filename);
        goto end;
    }
    
    dirpath = dirnamedup(user_path);
    compute_selected_tags(dirpath, &selected_tags);

    void remove_tag(const char *filename, void *tag, void *file)
    {
        untag_file(tag, file);
    }
    ht_for_each(selected_tags, &remove_tag, f);

    free(dirpath);
    free(filename);
    ht_free(selected_tags);
  end:
    return res;
}

int tag_rmdir(const char *user_path)
{
    int res = 0;
    int slash_count = get_character_count(user_path, '/');
    if (slash_count > 1) {
        res =  -EPERM;
        goto end;
    }
    
    char *tag = basenamedup(user_path);
    struct tag *t = tag_get(tag);
    if (NULL == t) {
        res = -ENOENT;
    } else {
        tag_remove(t);
    }
    free(tag);
  end:
    return res;
}

int tag_mkdir(const char *user_path, mode_t mode)
{
    int res = 0;
    int slash_count = get_character_count(user_path, '/');
    if (slash_count > 1) {
        res =  -EPERM;
        goto end;
    }
    char *filename = basenamedup(user_path);
    struct tag *t;
    t = tag_get(filename);
    if (INVALID_TAG != t) {
        res = -EEXIST;
        free(filename);
        goto end;
    }
    tag_get_or_create(filename);

    free(filename);
  end:
    return res;
}

static void readdir_list_files(
    void *buf, const char *user_path,
    struct hash_table *selected_tags, fuse_fill_dir_t filler)
{
    struct dirent *dirent;

    rewinddir(realdir);
    while ((dirent = readdir(realdir)) != NULL) {
        int res;
        struct stat stbuf;
        /* only list files, don't list directories */
        if (dirent->d_type == DT_DIR)
            continue;

        /* only files whose contents matches tags in path */
        char *virtpath = append_dir(user_path, dirent->d_name);
        char *realpath = append_dir(realdirpath, dirent->d_name);
        res = getattr_intra(
            virtpath, &stbuf, selected_tags, realpath, dirent->d_name);
        free(virtpath);
        free(realpath);
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

    for (int i = 1; i <= s; ++i) {
        struct tag *t = list_get(tagl, i);
        if (!ht_has_entry(selected_tags, t->value)) {
            filler(buf, t->value, NULL, 0);
        }
    }
}

/* list files within directory */
int tag_readdir(
    const char *user_path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
    struct hash_table *selected_tags;
    int res = 0;

    LOG("\n\nreaddir '%s'\n", user_path);

    compute_selected_tags(user_path, &selected_tags);
    readdir_list_tags(buf, selected_tags, filler);
    readdir_list_files(buf, user_path, selected_tags, filler);

    LOG("readdir returning %s\n\n\n", strerror(-res));

    return res;
}

static int read_tag_file(char *buffer, size_t len, off_t off)
{
    int res = 0;
    FILE *tagfile = tmpfile();
    tag_db_dump(tagfile);
    rewind(tagfile);
    res = fseek(tagfile, off, SEEK_SET);
    if (res < 0) {
        res = -errno;
        goto out;
    }
    res = fread(buffer, 1, len, tagfile);
    if (res < 0) {
        res = -errno;
    } else if (res < len) {
        DBG("res = %d ;; len = %d\n", res, len);
        for (int i = res; i < len; ++i) {
            buffer[i] = 0;
        }
    }
  out:
    fclose(tagfile);
    if (res < 0)
        LOG("read_tag returning '%s'\n", strerror(-res));
    else
        LOG("read_tag returning success (read %d)\n", res);
    return res;
}

/* read the content of the file */
int tag_read(
    const char *path, char *buffer, size_t len,
    off_t off, struct fuse_file_info *fi)
{
    struct filedes *filedes = (struct filedes*) fi->fh;
    int res, fd = filedes->fd;

    if (filedes->is_tagfile)
        return read_tag_file(buffer, len, off);

    LOG("read '%s' for %ld bytes starting at offset %ld\n", path, len, off);

    if (lseek(fd, off, SEEK_SET) < 0) {
        res = -errno;
        goto out;
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

  out:
    if (res < 0)
        LOG("read returning '%s'\n", strerror(-res));
    else
        LOG("read returning success (read %d)\n", res);
    return res;
}

int tag_write(
    const char *path, const char *buffer, size_t len,
    off_t off, struct fuse_file_info *fi)
{
    struct filedes *filedes = (struct filedes*) fi->fh;
    int res, fd = filedes->fd;
    
    if (filedes->is_tagfile)
        return -EPERM;

    LOG("write '%s' for %ld bytes starting at offset %ld\n",
        path, len, off);

    if (lseek(fd, off, SEEK_SET) < 0) {
        res = -errno;
        goto out;
    }
    res = write(fd, buffer, len);
    if (res < 0) {
        res = -errno;
    }

  out:
    if (res < 0)
        LOG("write returning '%s'\n", strerror(-res));
    else
        LOG("write returning success (wrote %d)\n", res);
    return res;
}

int tag_mknod(const char *user_path, mode_t mode, dev_t device)
{
    int res;
    char *realpath = tag_realpath(user_path);
    if (access(realpath, F_OK) == 0) {
        /* this condition allow user to have a more comprehensible error message
           when a (hidden)corresponding directory exist in the root directory */
        res = -EEXIST;
    } else {
        res = mknod(realpath, mode, device);
    }
    free(realpath);
    return res;
}

int tag_truncate(const char *user_path, off_t length)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = truncate(realpath, length);
    free(realpath);
    return res;
}

int tag_chmod(const char *user_path, mode_t mode)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = chmod(realpath, mode);
    free(realpath);
    return res;
}

int tag_chown(const char *user_path, uid_t user, gid_t group)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = chown(realpath, user, group);
    free(realpath);
    return res;
}

int tag_utime(const char *user_path, struct utimbuf *times)
{
    int res;
    char *realpath = tag_realpath(user_path);
    res = utime(realpath, times);
    free(realpath);
    return res;
}

int tag_symlink(const char *user_path, const char *tags)
{
    int res = 0;
    struct hash_table *selected_tags, *emptyhash = ht_create(0, NULL);
    struct stat stbuf;
    char *filename, *realpath;
    struct file *f;
    
    LOG("symlink file:'%s' - tags:'%s'\n", user_path, tags);
    
    realpath = tag_realpath(user_path);
    filename = basenamedup(user_path);
    tags = dirnamedup(tags);
    compute_selected_tags(tags, &selected_tags);
    
    res = getattr_intra(
        user_path, &stbuf, emptyhash, realpath, filename);
    if (res < 0)
        return res;
    f = file_get_or_create(filename);
    void tag_this_file(const char *tagname, void *tag, void* file)
    {
        struct file *f = file;
        struct tag *t = tag;
        tag_file(t, f);
    }
    ht_for_each(selected_tags, &tag_this_file, f);

    LOG("symlink returning '%s'\n", strerror(-res));
    return res;
}

    
struct fuse_operations tag_oper = {
    .open = tag_open,
    .release = tag_release,

    .getattr = tag_getattr,
    .fgetattr = tag_fgetattr,

    .read = tag_read,
    .readdir = tag_readdir,
    .write = tag_write,

    .symlink = tag_symlink,
    .readlink = tag_readlink,

    .unlink = tag_unlink,
    .rmdir = tag_rmdir,
    
    .mknod = tag_mknod,
    .mkdir = tag_mkdir,
    .truncate = tag_truncate,
    .chmod = tag_chmod,
    .chown = tag_chown,
    .utime = tag_utime,

};


