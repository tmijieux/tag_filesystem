#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>

#define __FILENAME__ (strrchr(__FILE__, '/') ?                  \
                      strrchr(__FILE__, '/') + 1 : __FILE__)

#define _(X) gettext((X))

void __print_error(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...);

#ifndef NO_LOG
void __print_log(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...);

#endif

#ifdef DEBUG

void __print_debug(
    const char *filename, int line,
    const char *pretty_function,const char *format, ...);
#endif


#define print_error(x, ...)  __print_error(                             \
    __FILENAME__, __LINE__, __PRETTY_FUNCTION__, x, ##__VA_ARGS__)

#ifndef NO_LOG
#    define print_log(x, ...)  __print_log(                             \
        __FILENAME__, __LINE__, __PRETTY_FUNCTION__, x, ##__VA_ARGS__)
#else
#    define print_log(f, ...)
#endif

#ifdef DEBUG
#    define print_debug(x, ...)  __print_debug(                         \
    __FILENAME__, __LINE__, __PRETTY_FUNCTION__, x, ##__VA_ARGS__)
#else
#    define print_debug(f, ...)
#endif

#endif //ERROR_H
