#include "list-thread-safe.h"

void init_list(list *l) {
    int err;
    l->head = NULL;

    if ((err = pthread_rwlock_init(&l->lock, NULL)) != 0)
        exit_with_err("pthread_rwlock_init", err);
}

void list_wlock(list *l) {
    int err;

    if ((err = pthread_rwlock_wrlock(&l->lock)) != 0)
        exit_with_err("pthread_rwlock_rdlock", err);
}

void list_rlock(list *l) {
    int err;

    if ((err = pthread_rwlock_rdlock(&l->lock)) != 0)
        exit_with_err("pthread_rwlock_rdlock", err);
}

void list_unlock(list *l) {
    int err;

    if ((err = pthread_rwlock_unlock(&l->lock)) != 0)
        exit_with_err("pthread_rwlock_unlock", err);
}

void list_insert(list *l, const char *key, const int value) {
    int err;

    node *n = malloc(sizeof(node));
    strncpy(n->key, key, KEY_SIZE);
    n->value = value;

    list_wlock(l);

    n->next = l->head;
    l->head = n;

    list_unlock(l);
}

void list_print(list *l) {
    list_rlock(l);

    node *ptr = l->head;

    while (ptr != NULL) {
        printf("(%s,%d) ", ptr->key, ptr->value);
        ptr = ptr->next;
    }

    list_unlock(l);

    printf("\n");
}

bool list_search(list *l, const char *key, int *result) {
    bool ret_value = 0;

    list_rlock(l);

    node *ptr = l->head;

    while (ptr != NULL && (strcmp(ptr->key, key) != 0))
        ptr = ptr->next;

    if (ptr != NULL) {
        ret_value = 1;
        *result = ptr->value;
    }

    list_unlock(l);

    return ret_value;
}

unsigned list_count(list *l) {
    int err;
    unsigned counter = 0;

    if ((err = pthread_rwlock_rdlock(&l->lock)) != 0)
        exit_with_err("pthread_rwlock_rdlock", err);

    node *ptr = l->head;

    while (ptr != NULL) {
        counter++;
        ptr = ptr->next;
    }

    if ((err = pthread_rwlock_unlock(&l->lock)) != 0)
        exit_with_err("pthread_rwlock_unlock", err);

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

    pthread_rwlock_destroy(&l->lock);
    free(l);
}