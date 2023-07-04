#include "hash-table-thread-safe.h"

hash_table *new_hash_table(unsigned long size) {
    int err;
    hash_table *h = malloc(sizeof(hash_table));

    h->size = size;
    h->n = 0;
    h->table = calloc(size, sizeof(item *));

    if ((err = pthread_rwlock_init(&h->lock, NULL)) != 0)
        exit_with_err("pthread_rwlock_init", err);

    return h;
}

unsigned long hash_function(const char *key) {
    unsigned const char *us;
    unsigned long h = 0;

    for (us = (unsigned const char *)key; *us; us++)
        h = h * HASH_FUNCTION_MULTIPLIER + *us;

    return h;
}

void hash_table_insert(hash_table *h, const char *key, const int value) {
    int err;
    item *i = malloc(sizeof(item));
    strncpy(i->key, key, KEY_SIZE);
    i->value = value;
    unsigned long hindex = hash_function(key);

    if ((err = pthread_rwlock_wrlock(&h->lock)) != 0)
        exit_with_err("pthread_rwlock_wrlock", err);

    hindex = hindex % h->size;
    i->next = h->table[hindex];
    h->table[hindex] = i;
    h->n++;

    if ((err = pthread_rwlock_unlock(&h->lock)) != 0)
        exit_with_err("pthread_rwlock_unlock", err);
}

bool hash_table_search(hash_table *h, const char *key, int *value) {
    int err;
    bool ret_value = 0;
    item *ptr;
    unsigned long hindex = hash_function(key);

    if ((err = pthread_rwlock_rdlock(&h->lock)) != 0)
        exit_with_err("pthread_rwlock_rdlock", err);

    hindex = hindex % h->size;
    ptr = h->table[hindex];

    while (ptr != NULL && strcmp(key, ptr->key))
        ptr = ptr->next;

    if (ptr != NULL) {
        ret_value = 1;
        *value = ptr->value;
    }

    if ((err = pthread_rwlock_unlock(&h->lock)) != 0)
        exit_with_err("pthread_rwlock_unlock", err);

    return ret_value;
}

unsigned long hash_table_get_n(hash_table *h) {
    int err;
    unsigned long n;

    if ((err = pthread_rwlock_rdlock(&h->lock)) != 0)
        exit_with_err("pthread_rwlock_rdlock", err);

    n = h->n;

    if ((err = pthread_rwlock_unlock(&h->lock)) != 0)
        exit_with_err("pthread-rwlock_unlock", err);

    return n;
}

void list_destroy(item *l) {
    item *ptr = l;
    item *tmp;

    while (ptr != NULL) {
        tmp = ptr;
        ptr = ptr->next;
        free(tmp);
    }
}

void hash_table_destroy(hash_table *h) {
    int err;

    if ((err = pthread_rwlock_wrlock(&h->lock)) != 0)
        exit_with_err("pthread_rwlock_wrlock", err);

    for (unsigned long i = 0; i < h->size; i++)
        list_destroy(h->table[i]);

    free(h->table);
    pthread_rwlock_destroy(&h->lock);
    free(h);
}