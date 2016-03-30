#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define RED_COLOR "31;1"
#define GREEN_COLOR "32"
#define RESET_COLOR "0"
#define COLOR(__c, __s) ESCAPE(__c) __s ESCAPE(RESET)
#define ESCAPE(__s) "\x1B[" __s##_COLOR "m"
                        
extern FILE *mylog;
#define LOG(args...) do { fprintf(mylog, args); fflush(mylog); } while (0)
#define ERROR(args...) do {                     \
        LOG("ERROR: " args);                    \
        exit(EXIT_FAILURE);                     \
    } while(0)

#ifdef DEBUG
#define DEBUGMSG(args...) do {                     \
        LOG("DEBUG: " args);                    \
    } while(0)
#else
#define DEBUGMSG(args...)
#endif

#endif //LOG_H
