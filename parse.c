#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "fuse_callback.h"
#include "tag.h"
#include "file.h"
#include "log.h"
#include "parse.h"

#define MAX_LENGTH 1000

static char *copy(char *word, char end)
{
    return strndup(word, strchr(word, end) - word);
}

void parse_tags(const char *filename)
{
    char word[MAX_LENGTH] = "";

    FILE *fi = NULL;
    fi = fopen(filename, "r+");
    struct file * f = NULL;
    if (fi != NULL) {
        while(fgets(word, MAX_LENGTH, fi) != NULL) {
            if (word[0] == '#')
                continue;
            char *data;
            struct stat st;
            int i = 0;
            while (isspace(word[i]))
                i++;
            if (word[i] == '[') {
                data = copy(word+i+1, ']');
                printf("file %s\n", data);

                if (tag_getattr(data, &st) >= 0)
                    f = file_get_or_create(data);
                else
                    print_error("file %s do not exist!\n", data);

            } else {
                data = copy(word+i, '\n');
                if (strlen(data)) {
                    struct tag *t = tag_get_or_create(data);
                    printf("tag %s\n", data);
                    if (f != NULL) {
                        tag_file(t, f);

                    }
                }
            }
            free(data);
        }
    }
    fclose(fi);
}
