#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>

// be sure to do this include first:
#define ERROR_H_IMPLEMENTATION__
#include "error.h"


static void print_any(
    char *string,
    const char *filename, int line,
    const char *pretty_function, const char *format, va_list ap)
{
    char *str;
    vasprintf(&str, format, ap);
    fprintf(stderr, _("\e[31;1m%s: %s\e[32m:\e[31;1m"
                      "%d\e[32m|\e[31;1m%s:\e[0m %s"),
            string, filename, line, pretty_function, str);
    free(str);
}

void print_error(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    print_any("ERROR", filename, line, pretty_function, format, ap);
}

void print_log(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    print_any("LOG", filename, line, pretty_function, format, ap);
}

#ifdef DEBUG

void print_debug(
    const char *filename, int line,
    const char *pretty_function,const char *format, ...)
{
    char *str;
    va_list ap;

    va_start(ap, format);
    vasprintf(&str, format, ap);
    fprintf(stderr, _("\e[6;30;43mDEBUG:\e[0;31;1m %s\e[32m:\e[31;1m"
                      "%d\e[32m|\e[31;1m%s:\e[0m %s"),
            filename, line, pretty_function, str);
    free(str);
}

#endif

