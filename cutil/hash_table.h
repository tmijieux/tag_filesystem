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

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>

struct hash_table *ht_create(size_t size, int (*hash)(const char*));
int ht_add_entry(struct hash_table *ht, const char *key, void *data);
int ht_add_unique_entry(struct hash_table *h, const char *key, void *data);
int ht_remove_entry(struct hash_table *ht, const char *key);
int ht_has_entry(struct hash_table *ht, const char *key);
int ht_get_entry(struct hash_table *ht, const char *key, void *ret);
void ht_free(struct hash_table *ht);
void ht_for_each(struct hash_table *ht,
                 void (*fun)(const char *, void*, void*), void *args);
struct list* ht_to_list(struct hash_table *ht);
int ht_entry_count(struct hash_table *ht);

#endif //HASH_TABLE_H
