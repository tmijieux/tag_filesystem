#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "cutil/error.h"

#define RED_COLOR "31;1"
#define GREEN_COLOR "32"
#define RESET_COLOR "0"

#define COLOR(__c, __s) ESCAPE(__c) __s ESCAPE(RESET)
#define ESCAPE(__s) "\x1B[" __s##_COLOR "m"
                        
extern FILE *mylog;

#define LOG(args...) do {                       \
        fprintf(mylog, args);                   \
        fflush(mylog);                          \
        print_log(args);                        \
    } while (0)

#define ERROR(args...) do {                     \
        LOG("ERROR: " args);                    \
        print_error(args);                      \
        exit(EXIT_FAILURE);                     \
    } while(0)

#ifdef DEBUG

#define DBG(args...) do {                       \
        print_debug(args);                      \
    } while(0)                                  \
        
#else
#define DEBUGMSG(args...)
#endif

#endif //LOG_H
