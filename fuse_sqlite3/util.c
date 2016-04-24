#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>

#include "cutil/error.h"
#include "util.h"

int get_character_count(const char *str, char c)
{
    int n = 0;
    for (int i = 0; str[i]; ++i)
        if (str[i] == c)
            ++n;
    return n;
}

char *basenamedup(const char *dir)
{
    char *res, *tmp = strdup(dir);
    res = strdup(basename(tmp));
    free(tmp);
    return res;
}

char *dirnamedup(const char *dir)
{
    char *res, *tmp = strdup(dir);
    res = strdup(dirname(tmp));
    free(tmp);
    return res;
}

char *append_dir(const char *dir, const char *file)
{
    char *str = NULL;
    if (asprintf(&str, "%s/%s", dir, file) < 0) {
        print_error("asprint allocation failed: %s\n", strerror(errno));
    }
    return str;
}
