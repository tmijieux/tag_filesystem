#include <string.h>
#include "tag.h"
#include "file.h"
#include "log.h"
#include "parse.h"

void update_lib(char * tagFile) {

	struct list* filesList = file_list();
	FILE * lib = NULL;
	lib = fopen(tagFile, "w+");

	if (lib != NULL) {
		for (int i = 1; i <= (int) list_size(filesList); i++) {
			void * data = list_get(filesList, i);
			struct file * fi = (struct file *) data;
			if (fi != NULL) {
				char * fileName = fi->name;
				fprintf(lib, "[%s]\n", fileName);
				struct hash_table * fiTags = fi->tags;
				struct list* fiTagsList = ht_to_list(fiTags);
				for (int j = 1; j <= (int) list_size(fiTagsList); j++) {
					struct tag * tg = (struct tag *) list_get(fiTagsList, j);
					if (tg != NULL) {
						char * tagValue = tg->value;
						fprintf(lib, "%s\n", tagValue);
					} else {
						printf("la structure tag n'existe pas\n");
					}
				}
			} else
				printf("la structure file n'existe pas\n");
		}
		fclose(lib);
	}
	
}