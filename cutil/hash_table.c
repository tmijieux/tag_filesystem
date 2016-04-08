/*
 *  Copyright (©) 2015 Lucas Maugère, Thomas Mijieux
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "uthash.h"
#include "hash_table.h"
#include "list.h"
#include "queue.h"

struct hash_table {
    char *key;
    void *data;

    UT_hash_handle hh;
    LIST_ENTRY(ht_entry) le;
    
};

int ht_entry_count(const hash_table *ht)
{
    return HASH_COUNT(*ht);
}

hash_table *ht_create(size_t size, int (*hash)(const char*))
{
    return calloc(sizeof(hash_table), 1);
}

int ht_add_entry(hash_table* ht, const char *key, void *data)
{
    struct hash_table *entry = malloc(sizeof*entry);
    entry->data = (void*) data;
    entry->key = strdup(key);
    HASH_ADD_STR(*ht, key, entry);
    return 0;
}

int ht_remove_entry(hash_table *ht, const char *key)
{
    struct hash_table *entry;
    HASH_FIND_STR(*ht, key, entry);
    if (NULL != entry) {
        HASH_DEL(*ht, entry);
        free(entry->key);
        free(entry);
        return 0;
    }
    return -1;
}

int ht_has_entry(hash_table *ht, const char *key)
{
    struct hash_table *entry;
    HASH_FIND_STR(*ht, key, entry);
    return entry != NULL;
}

int ht_get_entry(hash_table *ht, const char *key, void *ret)
{
    struct hash_table *entry;
    HASH_FIND_STR(*ht, key, entry);
    if (NULL != entry) {
        *((void**)ret) = entry->data;
        return 0;
    }
    return -1;
}

void ht_free(hash_table *ht)
{
    struct hash_table *entry, *tmp;
    HASH_ITER(hh, *ht, entry, tmp) {
        HASH_DEL(*ht, entry);
        free(entry->key);
        free(entry);
    }
}

void ht_for_each(hash_table* ht,
		 void (*fun)(const char *, void*, void*), void *args)
{
    struct hash_table *entry, *tmp;
    HASH_ITER(hh, *ht, entry, tmp) {
        fun(entry->key, entry->data, args);
    }
    free(ht);
}

struct list* ht_to_list(const hash_table *ht)
{
    struct hash_table *entry, *tmp;
    struct list *l = list_new(0);
    HASH_ITER(hh, *ht, entry, tmp) {
        list_add(l, entry->data);
    }
    return l;
}
