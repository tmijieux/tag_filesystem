#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data.h"

static FILE *rz_open_prefixed(
    const char *prefix, const char *path, const char *mode)
{
    char *full_path = rz_prefix_path(prefix, path);
    FILE *f = fopen(full_path, mode);
    free(full_path);

    return f;
}

FILE *rz_open_resource(const char *path, const char *mode)
{
    return rz_open_prefixed(PKG_DATA_DIR, path, mode);
}

FILE *rz_open_config(const char *path, const char *mode)
{
    return rz_open_prefixed(PKG_CONFIG_DIR, path, mode);
}

char *rz_prefix_path(const char *prefix, const char *path)
{
    size_t len = 1 + strlen(prefix) + strlen(path);
    char *prefixed_path = malloc(len);
    strcpy(prefixed_path, prefix);
    strcat(prefixed_path, path);
    return prefixed_path;
}

