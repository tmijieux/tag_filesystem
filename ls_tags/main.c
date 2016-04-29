#include <sys/types.h>
#include <sys/xattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>

#define _HEUR_SIZE 1024

static void usage(const char *imagefile)
{
    printf("usage: %s filename\n", imagefile);
}

static void ls_tags(const char *filename)
{
    char *buf = malloc(sizeof*buf * _HEUR_SIZE);
    ssize_t r, s;

    r = getxattr(filename, "tags", buf, _HEUR_SIZE);
    if (r < 0) {
        fprintf(stderr, "%s: erreur: %s\n", filename, strerror(errno));
        free(buf);
        return;
    }

    if (r > _HEUR_SIZE) {
        buf = realloc(buf, r+1);
        s = getxattr(filename, "tags", buf, r+1);
        if (s < 0) {
            perror(filename);
            free(buf);
            return;
        }
    }
    buf[r] = 0;
    printf("%s: %s\n", filename, buf);
    free(buf);
}

int main(int argc, char *const argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    setlocale(LC_ALL, "");
    for (int i = 1; i < argc; ++i)
        ls_tags(argv[i]);

    return EXIT_SUCCESS;
}

