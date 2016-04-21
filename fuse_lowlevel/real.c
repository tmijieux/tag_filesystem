#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>
#include <unistd.h>

#include "./real.h"
#include "./util.h"
#include "./parse.h"

DIR *realdir;
char *realdirpath;
char *tagdbpath;

int chdir(const char *path)
{
    int (*chdir_fn)(const char*);

    print_debug("chdir to %s\n", path);
    LOG("chdir to %s\n", path);
    chdir_fn = dlsym(RTLD_NEXT, "chdir");
    return chdir_fn(path);
}

void realdir_set_root(const char *path)
{
    if (NULL == realdirpath) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    realdir = opendir(realdirpath);
    if (!realdir) {
        fprintf(stderr, "%s: %s\n", realdirpath, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void realdir_add_files(void)
{
    struct dirent *dirent;

    rewinddir(realdir);
    while ((dirent = readdir(realdir)) != NULL) {
        // only list files, don't list directories
        if (dirent->d_type == DT_DIR)
            continue;
        file_get_or_create(dirent->d_name);
    }
}

void realdir_parse_tags(void)
{
    char *tag_file;
    int len = asprintf(&tag_file, "%s/%s", realdirpath, TAG_FILENAME);
    if (len < 0) {
        perror("asprintf");
        exit(EXIT_FAILURE);
    }
    print_debug("tag filename = %s\n", tag_file);
    parse_tags(tag_file);
    tagdbpath = tag_file;
}

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
