#define _GNU_SOURCE
#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include "log.h"

extern struct fuse_operations tag_oper;
extern DIR *dir;
extern char *dirpath;

static void set_root_directory(const char *path)
{
    dirpath = realpath(path, NULL);
    if (NULL == dirpath) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "%s: %s\n",
                dirpath, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

__attribute__((constructor))
static void init_locale(void)
{
    /* met le programme dans la langue du système */
    setlocale(LC_ALL, "");
}

int main(int argc, char *argv[])
{
    int err;
    
    if (argc < 2) {
        fprintf(stderr, "usage: %s target_directory mountpoint\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    /* find the absolute directory because fuse_main()
     * doesn't launch the daemon in the same current directory.
     */
    set_root_directory(argv[1]);
    argv++; argc--;
    LOG("\n");
    LOG("starting grepfs in %s\n", dirpath);
    err = fuse_main(argc, argv, &tag_oper, NULL);
    LOG("stopped grepfs with return code %d\n", err);

    closedir(dir);
    free(dirpath);

    LOG("\n");
    return err;
}
