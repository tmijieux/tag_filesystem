#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <pthread.h>

#include "uthash.h"
#include "hash_table.h"
#include "list.h"

#ifdef NO_MT

#define pthread_rwlock_rdlock(x)
#define pthread_rwlock_wrlock(x)
#define pthread_rwlock_unlock(x)
#define pthread_rwlock_init(x, y)
#define pthread_rwlock_destroy(x)

#endif

struct ht_entry {
    char *key;
    void *data;
    UT_hash_handle hh;
};

struct hash_table {
    struct ht_entry *h;
    #ifndef NO_MT
    pthread_rwlock_t rwlock;
    #endif
};

int ht_entry_count(struct hash_table *h)
{
    return HASH_COUNT(h->h);
}

struct hash_table *ht_create(size_t size, int (*hash)(const char*))
{
    struct hash_table *h = calloc(sizeof*h, 1);
    h->h = NULL;
    pthread_rwlock_init(&h->rwlock, NULL);
    return h;
}

int ht_add_entry(struct hash_table *h, const char *key, void *data)
{
    struct ht_entry *entry = malloc(sizeof*entry);
    entry->data = (void*) data;
    entry->key = strdup(key);

    pthread_rwlock_wrlock(&h->rwlock);
    HASH_ADD_STR(h->h, key, entry);
    pthread_rwlock_unlock(&h->rwlock);

    return 0;
}

int ht_add_unique_entry(struct hash_table *h, const char *key, void *data)
{
    int err = -1;
    struct ht_entry *entry;

    pthread_rwlock_wrlock(&h->rwlock);
    HASH_FIND_STR(h->h, key, entry);
    if (entry == NULL) {
        entry = malloc(sizeof*entry);
        entry->data = (void*) data;
        entry->key = strdup(key);
        HASH_ADD_STR(h->h, key, entry);
        err = 0;
    }
    pthread_rwlock_unlock(&h->rwlock);
    return err;
}

int ht_remove_entry(struct hash_table *h, const char *key)
{
    int err = -1;
    struct ht_entry *entry;

    pthread_rwlock_wrlock(&h->rwlock);
    HASH_FIND_STR(h->h, key, entry);
    if (entry != NULL) {
        HASH_DEL(h->h, entry);
        free(entry->key);
        free(entry);
        err = 0;
    }
    pthread_rwlock_unlock(&h->rwlock);

    return err;
}

int ht_has_entry(struct hash_table *h, const char *key)
{
    int res = 0;
    struct ht_entry *entry;

    pthread_rwlock_rdlock(&h->rwlock);
    HASH_FIND_STR(h->h, key, entry);
    res = (entry != NULL);
    pthread_rwlock_unlock(&h->rwlock);
    return res;
}

int ht_get_entry(struct hash_table *h, const char *key, void *ret)
{
    int err = -1;
    struct ht_entry *entry;

    pthread_rwlock_rdlock(&h->rwlock);
    HASH_FIND_STR(h->h, key, entry);
    if (NULL != entry) {
        *((void**)ret) = entry->data;
        err = 0;
    }
    pthread_rwlock_unlock(&h->rwlock);
    return err;
}

void ht_free(struct hash_table *h)
{
    struct ht_entry *entry, *tmp;
    HASH_ITER(hh, h->h, entry, tmp) {
        HASH_DEL(h->h, entry);
        free(entry->key);
        free(entry);
    }
    pthread_rwlock_destroy(&h->rwlock);
    free(h);
}

void ht_for_each(struct hash_table *h,
		 void (*fun)(const char *, void*, void*), void *args)
{
    struct ht_entry *entry, *tmp;

    pthread_rwlock_rdlock(&h->rwlock);
    HASH_ITER(hh, h->h, entry, tmp) {
        fun(entry->key, entry->data, args);
    }
    pthread_rwlock_unlock(&h->rwlock);
}

struct list* ht_to_list(struct hash_table *h)
{
    struct ht_entry *entry, *tmp;
    struct list *l = list_new(0);

    pthread_rwlock_rdlock(&h->rwlock);
    HASH_ITER(hh, h->h, entry, tmp) {
        list_add(l, entry->data);
    }
    pthread_rwlock_unlock(&h->rwlock);
    return l;
}
