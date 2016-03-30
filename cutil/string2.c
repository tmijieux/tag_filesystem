#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "string2.h"
#include "list.h"

#define HEURISTIC_SIZE 64

char *strdup2(const char *s)
{
    char *d = calloc(strlen(s) + 1, 1);
    strcpy(d, s);
    return d;
}

int asprintf(char **strp, const char *fmt, ...)
{
    va_list ap;

    char *buf = calloc(HEURISTIC_SIZE, 1);

    va_start(ap, fmt);
    int n = vsnprintf(buf, HEURISTIC_SIZE, fmt, ap);
    if (n >= HEURISTIC_SIZE) {
	buf = realloc(buf, n + 1);
	va_start(ap, fmt);	// important !!
	vsnprintf(buf, n + 1, fmt, ap);
    }

    *strp = buf;
    return n;
}

int character_is_in_string(int c, const char *str)
{
    int i;
    for (i = 0; str[i]; ++i)
	if (c == str[i])
	    return 1;
    return 0;
}

void strstripc(char *str, char c)
{
    size_t l = strlen(str);
    if (str[l - 1] == c)
	str[l - 1] = '\0';
}

char *str_replace_char(const char *str, char from, char to)
{
    char *rep = strdup(str);
    for (int i = 0; rep[i] != '\0'; ++i)
	if (rep[i] == from)
	    rep[i] = to;
    return rep;
}

char *strstrip(const char *str)
{
    char *strip_ = strdup(str);
    strstripc(strip_, '\n');
    return strip_;
}

uint32_t string_split(const char *str, const char *delim, char ***buf_addr)
{
    if (NULL == str) {
        *buf_addr = NULL;
        return 0;
    }

    char *strw = strdup(str);
    struct list *li = list_new(0);
    char *saveptr;
    char *p =  strtok_r(strw, delim, &saveptr);
    while (p != NULL) {
	list_add(li, strdup(p));
	p = strtok_r(NULL, delim, &saveptr);
    }
    free(strw);

    unsigned int s = list_size(li);
    if (!s) {
	*buf_addr = NULL;
    } else {
	*buf_addr = malloc(sizeof(*buf_addr) * s);
        for (unsigned i = 1; i <= s; ++i)
            (*buf_addr)[s - i] = list_get(li, i);
    }
    list_free(li);
    return s;
}


uint32_t string_split2(const char *str, const char *delim, char ***buf_addr)
{

    if (NULL == str) {
        *buf_addr = NULL;
        return 0;
    }

    struct list *li = list_new(0);
    int delim_length = strlen(delim);
    const char *str_ptr1, *str_ptr2;

    str_ptr1 = str;
    while (NULL != (str_ptr2 = strstr(str_ptr1, delim))) {
	list_add(li, strndup(str_ptr1, str_ptr2 - str_ptr1));
        str_ptr1 = str_ptr2 + delim_length;
    }
    list_add(li, strdup(str_ptr1));

    unsigned int s = list_size(li);
    if (!s) {
	*buf_addr = NULL;
    } else {
	*buf_addr = malloc(sizeof(*buf_addr) * s);
        for (unsigned i = 1; i <= s; ++i)
            (*buf_addr)[s - i] = list_get(li, i);
    }
    list_free(li);
    return s;
}


bool string_have_extension(const char *filename, const char *extension)
{
    size_t l = strlen(filename);
    size_t e = strlen(extension);
    if (l < e)
        return false;
    return (strcmp(extension, filename + l - e) == 0);
}
