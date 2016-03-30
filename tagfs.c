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

static DIR *dir;
static char *dirpath;

/*******************
 * Logs
 */

#define LOGFILE "tagfs.log"
FILE *mylog;
#define LOG(args...) do { fprintf(mylog, args); fflush(mylog); } while (0)

/*************************
 * File operations
 */

char *tag_realpath(const char *path)
{
    char *realpath;
    /* TODO remove directory names before the actual filename
     * so that grepped file '/foo/bar/toto' becomes 'toto'
     */
    /* prepend dirpath since the daemon runs in '/' */
    asprintf(&realpath, "%s/%s", dirpath, path);
    return realpath;
}

/* get attributes */
static int tag_getattr(const char *path, struct stat *stbuf)
{
    char *realpath = tag_realpath(path);
    int res;

    LOG("getattr '%s'\n", path);

    /* TODO try to stat the actual file */

    /* if the file doesn't exist, assume it's a virtual
       grepped directory and stat the main directory instead */
    res = stat(dirpath, stbuf);

    LOG("getattr returning %s\n", strerror(-res));
    free(realpath);
    return res;
}

/* list files within directory */
static int tag_readdir(
    const char *path, void *buf, fuse_fill_dir_t filler,
    off_t offset, struct fuse_file_info *fi)
{
    struct dirent *dirent;
    int res = 0;

    LOG("readdir '%s'\n", path);

    rewinddir(dir);
    while ((dirent = readdir(dir)) != NULL) {
        struct stat stbuf;
        res = tag_getattr(dirent->d_name, &stbuf);
        /* TODO only files whose contents matches tags in path */
        /* TODO only list files, don't list directories */
        filler(buf, dirent->d_name, NULL, 0);
    }

    LOG("readdir returning %s\n", strerror(-res));
    return 0;
}

/* read the grepped contents of the file */
int tag_read(
    const char *path, char *buffer, size_t len,
    off_t off, struct fuse_file_info *fi)
{
    char *realpath = tag_realpath(path);
    char *command;
    FILE *fd;
    char *cmdoutput;
    int res;

    LOG("read '%s' for %ld bytes starting at offset %ld\n", path, len, off);

    /* TODO if path is /foo/bar/toto, run "grep foo toto | grep bar"
     * TODO if path is /foo/toto, run "grep foo toto"
     */
    asprintf(&command, "cat %s", realpath);
    LOG("read using command %s\n", command);
  
    /* open the output of the command */
    fd = popen(command, "r");
    if (!fd) {
        res = -errno;
        goto out;
    }
  
    /* read up to len+off bytes from the command output */
    cmdoutput = malloc(len+off);
    if (!cmdoutput) {
        res = -ENOMEM;
        goto out_with_fd;
    }
    res = fread(cmdoutput, 1, len+off, fd);
    LOG("read got %d bytes out of %ld requested\n", res, len+off);
    if (res > off) {
        /* we read more than off, ignore the off
           first bytes and copy the remaining ones */
        memcpy(buffer, cmdoutput+off, res-off);
        res -= off;
    } else {
        /* we failed to read enough */
        res = 0;
    }

    free(cmdoutput);
    free(command);

  out_with_fd:
    pclose(fd);
  out:
    if (res < 0)
        LOG("read returning %s\n", strerror(-res));
    else
        LOG("read returning success (read %d)\n", res);
    free(realpath);
    return res;
}

static struct fuse_operations tag_oper = {
    .getattr = tag_getattr,
    .readdir = tag_readdir,
    .read = tag_read,
};

/**************************
 * Main
 */

int main(int argc, char *argv[])
{
    int err;

    if (argc < 2) {
        fprintf(stderr, "missing destination directory\n");
        exit(EXIT_FAILURE);
    }
    /* find the absolute directory because fuse_main()
     * doesn't launch the daemon in the same current directory.
     */
    dirpath = realpath(argv[1], NULL);
    dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "couldn't open directory %s\n", dirpath);
        exit(EXIT_FAILURE);
    }
    argv++;
    argc--;

    mylog = fopen(LOGFILE, "a"); /* append logs to previous executions */
    LOG("\n");

    LOG("starting grepfs in %s\n", dirpath);
    err = fuse_main(argc, argv, &tag_oper, NULL);
    LOG("stopped grepfs with return code %d\n", err);

    closedir(dir);
    free(dirpath);

    LOG("\n");
    return err;
}
