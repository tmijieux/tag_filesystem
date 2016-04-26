#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../cutil/error.h"

#define RED_COLOR "31;1"
#define GREEN_COLOR "32"
#define RESET_COLOR "0"

#define COLOR(__c, __s) ESCAPE(__c) __s ESCAPE(RESET)
#define ESCAPE(__s) "\x1B[" __s##_COLOR "m"

extern FILE *mylog;

#define ERROR(args...) do {                     \
        LOG("ERROR: " args);                    \
        print_error(args);                      \
        exit(EXIT_FAILURE);                     \
    } while(0)

#ifdef DEBUG

#define LOG(args...) do {                       \
        int save_errno;                         \
        save_errno = errno;                     \
        fprintf(mylog, args);                   \
        fflush(mylog);                          \
        print_log(args);                        \
        errno = save_errno;                     \
    } while (0)

#define DBG(args...) do {                       \
        print_debug(args);                      \
    } while(0)                                  \

#else

#define LOG(args...)
#define DBG(args...)

#endif

#endif //LOG_H
