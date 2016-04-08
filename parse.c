#include <string.h>
#include <ctype.h>

#include "tag.h"
#include "file.h"
#include "log.h"
#include "parse.h"

#define MAX_LENGTH 1000


static char *copy(char word[], char end)
{
    char *data;
    int length, j;
    
    for (length = 0; word[length] != end; length++);
    data = calloc(sizeof(char), length + 1);
    
    for (j= 0; j < length; j++)
        data[j] = word[j];
    data[j] = 0;
    
    return data;
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
            int i = 0;
            while (isspace(word[i]))
                i++;	
            if (word[i] == '[') {
                data = copy(word+i+1, ']');
                printf("file %s\n", data);
                f = file_get_or_create(data);

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
