#ifndef __GNUC__
#error "gcc vaincra"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1 /* must stay at the very top */
#endif
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>

/*
 * Ce programme mesures les performances de manipulation de plein de tags sur
 * un seul fichier.
 * Il doit être lancé DANS le répertoire racine du montage virtuel avec au
 * moins un fichier existant.
 *
 * Il mesure le temps pour réaliser les différentes opérations sur ce fichier,
 * pour 1, puis 2 tag, puis ... 2^N ... jusqu'a TAG_MAX.
 *
 * Il a besoin du support de mkdir, link, unlink et rmdir.
 * mkdir/rmdir pourront être enlevés si l'implémentation les crée à la volée.
 *
 * Lorsque l'opération permettant de lister les tags d'un fichier sera
 * implémentée, elle devra être mesurée selon le même principe.
 */

#define _BUF_SIZE 1024
#define TAGPREFIX "perfs_tmp_tag_"
#define TAG_MAX (64*1024)

/* up to 16^6 = 16 million tags actually supported in tag names */
#define TAG_PRINTF_FORMAT "%c%c%c%c%c%c"
#define TAG_PRINTF_ARGS(n)                      \
    (((n)>>20)&15)+'a', (((n)>>16)&15)+'a',     \
        (((n)>>12)&15)+'a', (((n)>>8)&15)+'a',  \
        (((n)>>4)&15)+'a',  (((n)>>0)&15)+'a'


#define ORDER_0(i) (i)
#define ORDER_1(i) (((i)+TAG_MAX/2)%TAG_MAX)
#define ORDER_2(i) (((i)+TAG_MAX/4)%TAG_MAX)
#define ORDER_3(i) (((i)+TAG_MAX/4*3)%TAG_MAX)

#define CHECK(v, n) do {if((v) < 0) {                   \
            perror(n); exit(EXIT_FAILURE);}}while(0)
#define DEF_CALLBACK(id, arg_f, arg_n, ...)                             \
    static inline void t_##id(const char *arg_f, const char *arg_n) {   \
        CHECK(id(__VA_ARGS__), #id); }

DEF_CALLBACK(mkdir, f, n, n, S_IRWXU);
DEF_CALLBACK(link, f, n, f, n);
DEF_CALLBACK(unlink, f, n, n);
DEF_CALLBACK(rmdir, f, n, n);

static inline void t_getxattr(const char *f, const char *n)
{
    char buf[_BUF_SIZE];
    CHECK(getxattr(n, "tags", buf, _BUF_SIZE), "getxattr");
}

static char *xasprintf(const char *fmt, ...)
{
    char *ret = NULL;
    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&ret, fmt, ap) < 0) {
        perror("asprintf");
        exit(EXIT_FAILURE);
    }
    va_end(ap);
    return ret;
}

static char *find_file(void)
{
    char *filename = NULL;
    struct dirent *dirent;
    DIR *dir = opendir(".");

    while ((dirent = readdir(dir)) != NULL) {
        struct stat stbuf;
        int err = stat(dirent->d_name, &stbuf);
        if (err < 0)
            continue;
        if (S_ISREG(stbuf.st_mode)) {
            filename = strdup(dirent->d_name);
            break;
        }
    }
    closedir(dir);

    if (!filename) {
        printf("couldn't find any file to tag\n");
        exit(EXIT_FAILURE);
    }
    printf("Will use file %s\n", filename);
    return filename;
}

/* static inline void DO_STEP(
 * int n, const char *fname, void (*cb)(const char*, const char*),
 * const char *op, const char *file_fmt, int (*order)(int)) */
#define DO_STEP(n, fname, cb, op, file_fmt, ord)                        \
    do {                                                                \
        struct timeval tv1, tv2;                                        \
                                                                        \
        gettimeofday(&tv1, NULL);                                       \
        for (int i = 0; i < n; ++i) {                                   \
            char *name = xasprintf(                                     \
                TAGPREFIX TAG_PRINTF_FORMAT file_fmt,                   \
                TAG_PRINTF_ARGS(ORDER_##ord(i)), fname);                \
            t_##cb(fname, name);                                        \
            free(name);                                                 \
            if (!(i & (i+1))) { /* i+1 power of two */                  \
                gettimeofday(&tv2, NULL);                               \
                unsigned long usecs = (tv2.tv_sec-tv1.tv_sec)*1000000+  \
                    (tv2.tv_usec-tv1.tv_usec);                          \
                printf("%lu usecs for %s %u tags\n", usecs, op, i+1);   \
            }                                                           \
        }                                                               \
    } while(0)

int main(int argc, char *argv[])
{
    if (argc > 1) {
        if (chdir(argv[1]) < 0) {
            perror(argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    char fs_type[_BUF_SIZE], cwd[_BUF_SIZE];
    getcwd(cwd, _BUF_SIZE);

    if (getxattr(cwd, "fs_type", fs_type, _BUF_SIZE) < 0
             || strcmp(fs_type, "fuse_tag") != 0) {
        fprintf(stderr, "%s: must be the tag filesystem\n", cwd);
        fprintf(stderr, "%s\n", fs_type);
        exit(EXIT_FAILURE);
    }

    char *f = find_file();
//    DO_STEP(TAG_MAX, f, mkdir, "creating", "", 0);
    DO_STEP(TAG_MAX, f, link, "adding", "/%s", 1);
// DO_STEP(TAG_MAX, f, getxattr, "listing", "/%s", 2);
    DO_STEP(TAG_MAX, f, unlink, "removing", "/%s", 2);
    DO_STEP(TAG_MAX, f, rmdir, "destroying", "", 3);

    return EXIT_SUCCESS;
}
