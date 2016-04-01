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
#include "tag.h"
#include "file.h"

extern struct fuse_operations tag_oper;
extern DIR *realdir;
extern char *realdirpath;

static void set_root_directory(const char *path)
{
    realdirpath = realpath(path, NULL);
    if (NULL == realdirpath) {
        fprintf(stderr, "%s: %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }
    realdir = opendir(realdirpath);
    if (!realdir) {
        fprintf(stderr, "%s: %s\n",
                realdirpath, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

__attribute__((constructor))
static void init_locale(void)
{
    /* met le programme dans la langue du système */
    setlocale(LC_ALL, "");
}


#ifdef DEBUG
static void manual_tag_for_test_purpose_only(void)
{
    struct file *f = file_get_or_create("log.c");
    struct tag *t = tag_get("paysage");
    tag_file(t, f);

    f = file_get_or_create("main.c");
    tag_file(t, f);
    t = tag_get("nuit");
    tag_file(t, f);
}
#endif

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
    #ifdef DEBUG
    manual_tag_for_test_purpose_only();
    #endif
    
    LOG("\n");
    LOG("starting grepfs in %s\n", realdirpath);
    err = fuse_main(argc, argv, &tag_oper, NULL);
    LOG("stopped grepfs with return code %d\n", err);

    closedir(realdir);
    free(realdirpath);

    LOG("\n");
    return err;
}
