#ifndef LIST_THREAD_UNSAFE_H
#define LIST_THREAD_UNSAFE_H
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
} list;

void init_list(list *l);
void list_insert(list *l, const char *key, const int value);
void list_print(const list *l);
bool list_search(const list *l, const char *key, int *result);
unsigned list_count(const list *l);
void list_destroy(list *l);

#endif