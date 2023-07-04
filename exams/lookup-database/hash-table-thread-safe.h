#ifndef HASH_TABLE_T
#define HASH_TABLE_T
#include "../../lib/lib-misc.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#define KEY_SIZE 4096
#define HASH_FUNCTION_MULTIPLIER 97

typedef struct __item {
    char key[KEY_SIZE];
    int value;
    struct __item *next;
} item;

typedef struct {
    unsigned long size;
    unsigned long n;
    item **table;
    pthread_rwlock_t lock;
} hash_table;

hash_table *new_hash_table(unsigned long size);
unsigned long hash_function(const char *key);
void hash_table_wlock(hash_table *h);
void hash_table_rlock(hash_table *h);
void hash_table_unlock(hash_table *h);
void hash_table_insert(hash_table *h, const char *key, const int value);
bool hash_table_search(hash_table *h, const char *key, int *value);
unsigned long hash_table_get_n(hash_table *h);
void hash_table_destroy(hash_table *h);

#endif