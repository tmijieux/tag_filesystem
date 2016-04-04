#ifndef UTIL_H
#define UTIL_H

#include "cutil/error.h"

int get_character_count(const char *str, char c);
char *basenamedup(const char *dir);
char *dirnamedup(const char *dir);
char *append_dir(const char *dir, const char *file);

#endif //UTIL_H
