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


#endif //LOG_H
