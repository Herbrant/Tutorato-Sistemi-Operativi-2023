#include "list.h"

void init_list(list *l) { l->head = NULL; }

void list_insert(list *l, const char *key, int value) {
    node *n = malloc(sizeof(node));
    strncpy(n->key, key, KEY_SIZE);
    n->value = value;

    if (l->head != NULL)
        n->next = l->head;
    else
        n->next = NULL;

    l->head = n;
}

void list_print(const list *l) {
    node *ptr = l->head;

    while (ptr != NULL) {
        printf("(%s,%d) ", ptr->key, ptr->value);
        ptr = ptr->next;
    }
}

bool list_search(const list *l, const char *key, int *result) {
    node *ptr = l->head;

    while (ptr != NULL && (strcmp(ptr->key, key) != 0))
        ptr = ptr->next;

    if (ptr == NULL)
        return 0;

    *result = ptr->value;
    return 1;
}

int list_count(const list *l) {
    node *ptr = l->head;
    int counter = 0;

    while (ptr != NULL) {
        counter++;
        ptr = ptr->next;
    }

    return counter;
}

void list_destroy(list *l) {
    node *ptr = l->head;
    node *tmp;

    while (ptr != NULL) {
        tmp = ptr;
        ptr = ptr->next;
        free(tmp);
    }

    free(l);
}