#include "sys.h"
#include "util.h"
#include "fuse_callback.h"

#include "tag.h"
#include "filedes.h"
#include "file.h"
#include "tagio.h"

static bool file_matches_tags(
    const char *filename, struct hash_table *selected_tags)
{
    struct file *f = file_get_or_create(filename);
    bool match = true;
    /* viva el gcc: */
    void check_tags(const char *name, void *tag, void *match_) {
        bool *match = match_;
        struct tag *t = tag;
        if (t == INVALID_TAG || !ht_has_entry(f->tags, t->value))
            *match = false;
    }
    print_debug("selected tags size: %d\n", ht_entry_count(selected_tags));
    ht_for_each(selected_tags, &check_tags, &match);
    return match;
}

int tag_open(const char *user_path, struct fuse_file_info *fi)
{
    int res = 0;
    struct filedes *fd = filedes_create(user_path);
    struct file *f = file_get_or_create(fd->filename);
    res = file_open(f, fi->flags);
    fi->fh = (uint64_t) fd;
    return res;
}

int tag_release(const char *user_path, struct fuse_file_info *fi)
{
    struct filedes *fd = (struct filedes*) fi->fh;
    struct file *f = file_get_or_create(fd->filename);
    file_close(f);
    filedes_delete(fd);
    return 0;
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
            }
        } else if (!strcmp(filename, TAG_FILENAME)) {
            /* tag file is special and have "almost infinite" size */
            stbuf->st_size = (off_t) LONG_MAX;
        }
    }
    return res;
}

/* get attributes */
int tag_getattr(const char *user_path, struct stat *stbuf)
{
    int res;

    LOG("getattr '%s'\n", user_path);

    struct filedes *fd = filedes_create(user_path);

    res = getattr_intra(
        user_path, stbuf, fd->selected_tags, fd->realpath, fd->filename);

    LOG("getattr returning '%s'\n", strerror(-res));

    filedes_delete(fd);
    return res;
}

int tag_fgetattr(
    const char *user_path, struct stat *stbuf, struct fuse_file_info *fi)
{
    struct filedes *fd = (struct filedes*)fi->fh;
    return getattr_intra(
        user_path, stbuf, fd->selected_tags, fd->realpath, fd->filename);
}


static int fd_unlink(struct filedes *fd)
{
    struct file *f;

    f = file_get(fd->filename);
    if (NULL == f) {
        return -ENOENT;
    }
    void remove_tag(const char *filename, void *tag, void *file)
    {
        untag_file(tag, file);
    }
    ht_for_each(fd->selected_tags, &remove_tag, f);
    return 0;
}

int tag_unlink(const char *user_path)
{
    int res = 0;
    int slash_count = get_character_count(user_path, '/');

    LOG("unlink '%s'\n", user_path);
    if (slash_count <= 1)
        return -EPERM;

    struct filedes *fd = filedes_create(user_path);
    res = fd_unlink(fd);

    filedes_delete(fd);
    return res;
}

int tag_rmdir(const char *user_path)
{
    int res = 0;
    int slash_count = get_character_count(user_path, '/');
    LOG("rmdir '%s'\n", user_path);
    if (slash_count > 1)
        return -EPERM;

    char *tag = basenamedup(user_path);
    struct tag *t = tag_get(tag);
    if (NULL == t) {
        res = -ENOENT;
    } else {
        tag_remove(t);
    }
    free(tag);
    return res;
}

int tag_mkdir(const char *user_path, mode_t mode)
{
    int res = 0;
    LOG("mkdir '%s'\n", user_path);
    int slash_count = get_character_count(user_path, '/');
    if (slash_count > 1)
        return  -EPERM;

    char *filename = basenamedup(user_path);
    struct tag *t;
    t = tag_get(filename);
    if (INVALID_TAG != t) {
        res = -EEXIST;
    } else {
        tag_get_or_create(filename);
    }
    free(filename);
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

static void readdir_list_tags_mode1(
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

static struct hash_table *readdir_get_selected_files(
    struct hash_table *selected_tags)
{
    struct hash_table *selected_files;
    selected_files = ht_create(0, NULL);
    // 1 get all file for selected tags:
    void each_tag(const char *n, void *tag, void *arg)
    {
        void each_file(const char *n, void *file, void *arg)
        {
            struct file *f = file;
            ht_add_unique_entry(selected_files, f->name, f);
        }
        struct tag *t = tag;
        ht_for_each(t->files, &each_file, NULL);
    }
    ht_for_each(selected_tags, &each_tag, NULL);

    // remove file not matching all the tags
    void each_file(const char *n, void *file, void *arg)
    {
        void each_tag(const char *n, void *tag, void *file)
        {
            struct file *f = file;
            struct tag *t = tag;
            if (!ht_has_entry(f->tags, t->value)) {
                ht_remove_entry(selected_files, f->name);
                print_debug("removing file %s from selected_files\n", f->name);
            }
        }
        ht_for_each(selected_tags, &each_tag, file);
    }
    ht_for_each(selected_files, &each_file, NULL);

    return selected_files;
}

static void readdir_fill_remaining_tags(
    struct hash_table *selected_tags, struct hash_table *selected_files,
    void *buf, fuse_fill_dir_t filler)
{
    struct hash_table *remaining_tags;
    remaining_tags = ht_create(0, NULL);
    void each_file(const char *n, void *file, void *arg)
    {
        void each_tag(const char *n, void *tag, void *arg)
        {
            struct tag *t = tag;
            if (!ht_has_entry(selected_tags, t->value) &&
                !ht_has_entry(remaining_tags, t->value))
            {
                filler(buf, t->value, NULL, 0);
                ht_add_unique_entry(remaining_tags, t->value, t);
            }
        }
        struct file *f = file;
        ht_for_each(f->tags, &each_tag, NULL);
    }
    ht_for_each(selected_files, &each_file, NULL);

    ht_free(remaining_tags);
}

static void readdir_list_tags_mode2(
    void *buf, struct hash_table *selected_tags, fuse_fill_dir_t filler)
{
    // recuperer les tags possibles pour les fichiers correspondant au
    // tags selectionnés

    // 1 etape: recuperer les fichiers correspondant au tag selectionnés:
    struct hash_table *selected_files;
    selected_files = readdir_get_selected_files(selected_tags);

    // 2 eme etape: afficher tous les tags non selectionnés parmi les tags de
    // ces fichiers:
    readdir_fill_remaining_tags(
        selected_tags, selected_files, buf, filler);
    ht_free(selected_files);
    // peut etre faudrait il le cacher au lieu de le liberer ici
}


/* list files within directory */
int tag_readdir(
    const char *user_path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
    struct hash_table *selected_tags;
    int res = 0;

    LOG("\n\nreaddir '%s'\n", user_path);

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    compute_selected_tags(user_path, &selected_tags);
    if (ht_entry_count(selected_tags) > 0)
        readdir_list_tags_mode2(buf, selected_tags, filler);
    else
        readdir_list_tags_mode1(buf, selected_tags, filler);
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
    int res = 0;
    struct filedes *fd = (struct filedes*) fi->fh;
    struct file *f = file_get_or_create(fd->filename);

    if (fd->is_tagfile)
        return read_tag_file(buffer, len, off);

    LOG("read '%s' for %ld bytes starting at offset %ld\n", path, len, off);
    res = file_read(f, buffer, len, off);
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
    int res = 0;
    struct filedes *fd = (struct filedes*) fi->fh;
    struct file *f = file_get_or_create(fd->filename);

    if (fd->is_tagfile)
        return -EPERM;

    LOG("write '%s' for %ld bytes starting at offset %ld\n", path, len, off);
    res = file_write(f, buffer, len, off);
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

static int fd_link(struct filedes *ffrom, struct filedes *fto)
{
    int res;
    struct stat stbuf;
    struct file *f;

    res = getattr_intra(
        ffrom->virtpath, &stbuf, ffrom->selected_tags,
        ffrom->realpath, ffrom->filename);
    if (res < 0)
        return res;
    f = file_get_or_create(ffrom->filename);
    void tag_this_file(const char *tagname, void *tag, void* file)
    {
        tag_file(tag, file);
    }
    ht_for_each(fto->selected_tags, &tag_this_file, f);
    return res;
}

int tag_link(const char *from, const char *to)
{
    int res = 0;
    LOG("link file from:'%s' - to:'%s'\n", from, to);

    struct filedes *ffrom = filedes_create(from);
    struct filedes *fto = filedes_create(to);

    res = fd_link(ffrom, fto);

    filedes_delete(ffrom);
    filedes_delete(fto);

    LOG("link returning '%s'\n", strerror(-res));
    return res;
}

int tag_rename(const char *from, const char *to)
{
    int res = 0;
    struct filedes *ffrom = filedes_create(from);
    struct filedes *fto = filedes_create(to);

    if (ht_entry_count(ffrom->selected_tags) != 1 ||
        ht_entry_count(fto->selected_tags) != 1)
    {
        res =  -EPERM;
    }
    else
    {
        fd_unlink(ffrom);
        fd_link(ffrom, fto);
    }

    filedes_delete(ffrom);
    filedes_delete(fto);

    return res;
}

static int ioctl_read_tags(struct filedes *fd, void *data_)
{
    int len;
    char *str;
    struct tag_ioctl *data = data_;
    struct file *f = file_get(fd->filename);
    if (NULL == f)
        return -ENOENT;
    
    str = file_get_tags_string(f, &len);
    print_debug("file tag string : %s\n", str);
    strncpy(data->buf, str, BUFSIZE);
    data->buf[BUFSIZE] = '\0';

    print_debug("copied data : %s\n", data->buf);
    if (len > BUFSIZE) {
        int id = 0;//tag_ioctl_unique_id();
        data->again = 1;
        data->size = BUFSIZE;
        data->id = id;
    } else {
        data->again = 0;
        data->size = len;
    }
    free(str);
    return 0;
}

int tag_ioctl(
    const char *path, int cmd, void *arg,
    struct fuse_file_info *fi, unsigned int flags, void *data)
{
    (void) flags;
    struct filedes *fd = (struct filedes*) fi->fh;
        
    if (fd->is_tagfile || fd->is_directory)
        return -EINVAL;
    if (flags & FUSE_IOCTL_COMPAT)
        return -ENOSYS;
    
    switch (cmd) {
    case TAGIOC_READ_TAGS:
        return ioctl_read_tags(fd, data);
        break;
    }
    return -EINVAL;
}

struct fuse_operations tag_oper = {
    .open = tag_open,
    .release = tag_release,

    .getattr = tag_getattr,
    .fgetattr = tag_fgetattr,

    .read = tag_read,
    .readdir = tag_readdir,
    .write = tag_write,

    .link = tag_link,
    .unlink = tag_unlink,
    .rmdir = tag_rmdir,

    .mknod = tag_mknod,
    .mkdir = tag_mkdir,
    .truncate = tag_truncate,
    .chmod = tag_chmod,
    .chown = tag_chown,
    .utime = tag_utime,
    .rename = tag_rename,

    .ioctl = tag_ioctl,

    .flag_nullpath_ok = 0,
};


