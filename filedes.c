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

struct filedes *filedes_create(const char *user_path)
{
    struct filedes *fd = calloc(sizeof *fd, 1);

    fd->is_directory = false;
    fd->is_tagfile = false;
    fd->realpath = tag_realpath(user_path);
    fd->virtpath = strdup(user_path);
    fd->virtdirpath = dirnamedup(user_path);
    fd->filename = basenamedup(user_path);
    if (!strcmp(fd->filename, ".tags"))
        fd->is_tagfile = true;
    compute_selected_tags(fd->virtdirpath, &fd->selected_tags);

    return fd;
}

void filedes_delete(struct filedes *fd)
{
    free((char*) fd->realpath);
    free((char*) fd->virtpath);
    free((char*) fd->virtdirpath);
    free((char*) fd->filename);
    ht_free(fd->selected_tags);
}
