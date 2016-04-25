#include <sys/types.h>
#include <sys/xattr.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _HEUR_SIZE 1024

static void usage(const char *imagefile)
{
    printf("usage: %s filename\n", imagefile);
}

int main(int argc, char *const argv[])
{
    if (argc < 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];
    char *buf = malloc(sizeof*buf * _HEUR_SIZE);
    ssize_t r, s;

    r = getxattr(filename, "tags", buf, _HEUR_SIZE);
    if (r < 0) {
        perror("getxattr");
        exit(EXIT_FAILURE);
    }

    if (r > _HEUR_SIZE) {
        buf = realloc(buf, r+1);
        s = getxattr(argv[1], "tags", buf, r+1);
        if (s < 0) {
            perror("getxattr");
            exit(EXIT_FAILURE);
        }
    } 
    buf[r] = 0;
    printf("%s: %s\n", filename, buf);
    return EXIT_SUCCESS;
}

