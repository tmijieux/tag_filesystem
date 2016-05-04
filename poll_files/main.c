#include <sys/types.h>
#include <sys/xattr.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define _HEUR_SIZE 1024

/*
static void usage(const char *imagefile)
{
    printf("usage: %s filename\n", imagefile);
}
*/

static void check_event(struct pollfd *pf, const char *filename)
{
    if (pf->revents & POLLIN)
        printf("ajout d'un tag sur %s\n", filename);

    if (pf->revents & POLLOUT)
        printf("suppression d'un tag sur %s\n", filename);

    pf->revents = 0;
}

int main(int argc, char *const argv[])
{
    setlocale(LC_ALL, "");

    char *name[] = { "ipb.jpeg", "bilbo.jpeg" };
    int size = sizeof name / sizeof *name;

    int f[size];
    struct pollfd fds[size];

    for (int i = 0; i < size; ++i) {
        f[i] = open(name[i], O_RDWR);
        if (f[i] < 0) {
            perror(name[i]);
            f[i] = -1;
        }
        fds[i].fd = f[i];
        fds[i].events = POLLIN|POLLOUT;
        fds[i].revents = 0;
    }

    while (true) {
        int err = poll(fds, size, -1);
        if (err < 0) {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < size; ++i)
            check_event(&fds[i], name[i]);
    }
    return EXIT_SUCCESS;
}

