#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "uthash.h"
#include "hash_table.h"
#include "list.h"
#include "queue.h"

struct ht_entry {
    char *key;
    void *data;

    UT_hash_handle hh;
};

struct hash_table {
    struct ht_entry *h;
};

int ht_entry_count(const struct hash_table *h)
{
    return HASH_COUNT(h->h);
}

struct hash_table *ht_create(size_t size, int (*hash)(const char*))
{
    struct hash_table *h = calloc(sizeof*h, 1);
    h->h = NULL;
    return h;
}

int ht_add_entry(struct hash_table *h, const char *key, void *data)
{
    struct ht_entry *entry = malloc(sizeof*entry);
    entry->data = (void*) data;
    entry->key = strdup(key);
    HASH_ADD_STR(h->h, key, entry);
    return 0;
}

int ht_add_unique_entry(struct hash_table *h, const char *key, void *data)
{
    struct ht_entry *entry;
    HASH_FIND_STR(h->h, key, entry);
    if (NULL != entry)
        return -1;
    entry = malloc(sizeof*entry);
    entry->data = (void*) data;
    entry->key = strdup(key);
    HASH_ADD_STR(h->h, key, entry);
    return 0;
}


int ht_remove_entry(struct hash_table *h, const char *key)
{
    struct ht_entry *entry;
    HASH_FIND_STR(h->h, key, entry);
    if (NULL != entry) {
        HASH_DEL(h->h, entry);
        free(entry->key);
        free(entry);
        return 0;
    }
    return -1;
}

int ht_has_entry(struct hash_table *h, const char *key)
{
    struct ht_entry *entry;
    HASH_FIND_STR(h->h, key, entry);
    return entry != NULL;
}

int ht_get_entry(struct hash_table *h, const char *key, void *ret)
{
    struct ht_entry *entry;
    HASH_FIND_STR(h->h, key, entry);
    if (NULL != entry) {
        *((void**)ret) = entry->data;
        return 0;
    }
    return -1;
}

void ht_free(struct hash_table *h)
{
    struct ht_entry *entry, *tmp;
    HASH_ITER(hh, h->h, entry, tmp) {
        HASH_DEL(h->h, entry);
        free(entry->key);
        free(entry);
    }
    free(h);
}

void ht_for_each(struct hash_table *h,
		 void (*fun)(const char *, void*, void*), void *args)
{
    struct ht_entry *entry, *tmp;
    HASH_ITER(hh, h->h, entry, tmp) {
        fun(entry->key, entry->data, args);
    }
}

struct list* ht_to_list(const struct hash_table *h)
{
    struct ht_entry *entry, *tmp;
    struct list *l = list_new(0);
    HASH_ITER(hh, h->h, entry, tmp) {
        list_add(l, entry->data);
    }
    return l;
}
