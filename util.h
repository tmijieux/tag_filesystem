#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "log.h"

#include "cutil/error.h"
#include "cutil/string2.h"
#include "cutil/hash_table.h"
#include "cutil/list.h"

#define min(a, b) ((a) < (b) ? (a) : (b))

int get_character_count(const char *str, char c);
char *basenamedup(const char *dir);
char *dirnamedup(const char *dir);
char *append_dir(const char *dir, const char *file);

#endif //UTIL_H
