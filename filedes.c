#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <unistd.h>

#include "filedes.h"
#include "util.h"


DIR *realdir;
char *realdirpath;

char *tag_realpath(const char *user_path)
{
    char *realpath, *file;
    /* remove directory names before the actual filename
     * so that grepped file '/foo/bar/toto' becomes 'toto'
     */
    file = basenamedup(user_path);

    /* prepend dirpath since the daemon runs in '/' */
    if (asprintf(&realpath, "%s/%s", realdirpath, file) < 0) {
        print_error("asprintf: allocation failed: %s", strerror(errno));
    }
    free(file);
    return realpath;
}

int filedes_create(const char *user_path, int flags, struct filedes **fd__)
{
    int res = 0;
    struct filedes *fd = calloc(sizeof *fd, 1);
    
    fd->realpath = tag_realpath(user_path);
    fd->fd = open(fd->realpath, flags);
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

    *fd__ = fd;
    return res;
}


int filedes_delete(struct filedes *fd)
{
    int res = 0;
    
    free((char*) fd->realpath);
    free((char*) fd->virtpath);
    free((char*) fd->virtdirpath);
    ht_free(fd->selected_tags);
    res = close(fd->fd);
    free(fd);
    return res;
}
