#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../tagio.h"

#define BUFSIZE 1024

void print_tag_list(const char *filename)
{
    int fd = open(filename, O_RDWR);
    if (fd < 0) {
        perror(filename);
        return;
    }
    struct tag_ioctl_rw_arg io;
    io.buf = malloc(BUFSIZE);
    io.size = BUFSIZE;
    
    ioctl(fd, TAGIOC_READ_TAGS, &io);
    if (io.total_size > io.size) {
        io.buf = realloc(io.buf, io.total_size+1);
        io.size = io.total_size+1;
        ioctl(fd, TAGIOC_READ_TAGS, &io);
    }
    printf("%s: %s\n", filename, (char*)io.buf);
    free(io.buf);
    close(fd);
}


int main(int argc, char *const argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s filename1 [ filename2 ... ]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i) {
        print_tag_list(argv[i]);
    }
    return EXIT_SUCCESS;
}
