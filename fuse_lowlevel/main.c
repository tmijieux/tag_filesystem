#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#include "parse.h"
#include "log.h"
#include "tag.h"
#include "file.h"
#include "fuse_callback.h"

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
    struct tag *t = tag_get_or_create("paysage");
    tag_file(t, f);

    f = file_get_or_create("main.c");
    tag_file(t, f);
    t = tag_get_or_create("nuit");
    tag_file(t, f);
}
#endif

int lowlevel_main(int argc, char *argv[], void *user_data)
{
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    struct fuse_chan *ch;
    char *mountpoint;
    int foreground, multithreaded;
    
    int err = -1;
    if (fuse_parse_cmdline(&args, &mountpoint,
                           &multithreaded, &foreground) != -1 &&
        (ch = fuse_mount(mountpoint, &args)) != NULL &&
        fuse_daemonize(foreground) != -1)
    {
        struct fuse_session *se;
        se = fuse_lowlevel_new(&args, &tag_oper, sizeof(tag_oper), user_data);
        if (se != NULL) {
            if (fuse_set_signal_handlers(se) != -1) {
                fuse_session_add_chan(se, ch);
                /* Block until ctrl+c or fusermount -u */
                if (multithreaded == 1)
                    err = fuse_session_loop_mt(se);
                else
                    err = fuse_session_loop(se);
                fuse_remove_signal_handlers(se);
                fuse_session_remove_chan(ch);
            }
            fuse_session_destroy(se);
        }
        fuse_unmount(mountpoint, ch);
    }
    fuse_opt_free_args(&args);
    return err;
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

    char *progname = argv[0];
    char *root_directory = argv[1];
    realdirpath = realpath(root_directory, NULL);
    argv++; argc--;
    #ifdef DEBUG
//    manual_tag_for_test_purpose_only();
    #endif

    LOG("starting %s in %s\n", progname, root_directory);
    err = lowlevel_main(argc, argv, root_directory);
    LOG("stopped %s with return code %d\n\n", progname, err);

    closedir(realdir);
    free(realdirpath);

    return err;
}
