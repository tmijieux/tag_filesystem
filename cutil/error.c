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
#include "error.h"
#include "log.h"

static void print_any(
    FILE *f, char *string,
    const char *filename, int line,
    const char *pretty_function, const char *format, va_list ap)
{
    char *str = NULL;
    if (vasprintf(&str, format, ap) < 0)
        perror("vasprintf: ");

    fprintf(f, "\e[31;1m%s: %s\e[32m:\e[31;1m"
            "%d\e[32m|\e[31;1m%s:\e[0m %s",
            string, filename, line, pretty_function, str);
    free(str);
}

static void print_file(
    FILE *f, char *string,
    const char *filename, int line,
    const char *pretty_function, const char *format, va_list ap)
{
    char *str = NULL;
    if (vasprintf(&str, format, ap) < 0)
        perror("vasprintf: ");

    fprintf(f, "\%s: %s: %d|%s: %s",
            string, filename, line, pretty_function, str);
    free(str);
}

void __print_error(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    print_any(stderr, "ERROR", filename, line,
              pretty_function, format, ap);
}


#ifndef NO_LOG
void __print_log(
    const char *filename, int line,
    const char *pretty_function, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    print_file(mylog, "LOG", filename, line, pretty_function, format, ap);

    va_start(ap, format);
    print_any(stderr, "LOG", filename, line, pretty_function, format, ap);
}

#endif

#ifdef DEBUG

void __print_debug(
    const char *filename, int line,
    const char *pretty_function,const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    print_any(stderr, _("\e[6;30;43mDEBUG:\e[0;0;0m"),
            filename, line, pretty_function, format, ap);
}

#endif

