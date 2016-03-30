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

void rz_error(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...);

#ifndef ERROR_H_IMPLEMENTATION__
# define rz_error(x, ...)  rz_error(                                    \
    __FILENAME__, __LINE__, __PRETTY_FUNCTION__, x, ##__VA_ARGS__)
#endif

#ifdef DEBUG

void rz_debug(
    const char *filename, int line,
    const char *pretty_function,const char *format, ...);

# ifndef ERROR_H_IMPLEMENTATION__
#  define rz_debug(x, ...)  rz_debug(                                   \
    __FILENAME__, __LINE__, __PRETTY_FUNCTION__, x, ##__VA_ARGS__)
# endif

#else

# define rz_debug(f, ...)

#endif

#endif //ERROR_H
