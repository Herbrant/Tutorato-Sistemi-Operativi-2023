#ifndef LIST_H
#define LIST_H
#include "../../lib/lib-misc.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KEY_SIZE 4096

typedef struct __node {
    char key[KEY_SIZE];
    int value;
    struct __node *next;
} node;

typedef struct {
    node *head;
    pthread_rwlock_t lock;
} list;

void init_list(list *l);
void list_wlock(list *l);
void list_rlock(list *l);
void list_unlock(list *l);
void list_insert(list *l, const char *key, const int value);
void list_print(list *l);
bool list_search(list *l, const char *key, int *result);
unsigned list_count(list *l);
void list_destroy(list *l);

#endif