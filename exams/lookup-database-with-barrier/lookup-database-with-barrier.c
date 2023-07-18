#include "../../lib/lib-misc.h"
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define KEY_SIZE 2048
#define BUFFER_SIZE 4096
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
    pthread_mutex_t lock;
} hash_table;

hash_table *new_hash_table(unsigned long size) {
    int err;
    hash_table *h = malloc(sizeof(hash_table));

    h->size = size;
    h->n = 0;
    h->table = calloc(size, sizeof(item *));

    if ((err = pthread_mutex_init(&h->lock, NULL)) != 0)
        exit_with_err("pthread_mutex_init", err);

    return h;
}

void hash_table_lock(hash_table *h) {
    int err;

    if ((err = pthread_mutex_lock(&h->lock)) != 0)
        exit_with_err("pthread_mutex_lock", err);
}

void hash_table_unlock(hash_table *h) {
    int err;

    if ((err = pthread_mutex_unlock(&h->lock)) != 0)
        exit_with_err("pthread_mutex_unlock", err);
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
    unsigned long hindex = hash_function(key) % h->size;

    hash_table_lock(h);

    i->next = h->table[hindex];
    h->table[hindex] = i;
    h->n++;

    hash_table_unlock(h);
}

bool hash_table_search(hash_table *h, const char *key, int *value) {
    int err;
    bool ret_value = 0;
    item *ptr;
    unsigned long hindex = hash_function(key) % h->size;

    ptr = h->table[hindex];

    while (ptr != NULL && strcmp(key, ptr->key))
        ptr = ptr->next;

    if (ptr != NULL) {
        ret_value = 1;
        *value = ptr->value;
    }

    return ret_value;
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

    hash_table_lock(h);

    for (unsigned long i = 0; i < h->size; i++)
        list_destroy(h->table[i]);

    free(h->table);
    pthread_mutex_destroy(&h->lock);
    free(h);
}

typedef struct {
    hash_table *h;
    pthread_barrier_t barrier;
    unsigned nthread;
} shared_db;

void init_shared_db(shared_db *sdb, unsigned nthread) {
    int err;

    sdb->h = new_hash_table(1024);
    sdb->nthread = nthread;

    if ((err = pthread_barrier_init(&sdb->barrier, NULL, nthread)) != 0)
        exit_with_err("pthread_barrier_init", err);
}

void destroy_shared_db(shared_db *sdb) {
    hash_table_destroy(sdb->h);
    pthread_barrier_destroy(&sdb->barrier);
    free(sdb);
}

typedef struct {
    unsigned thread_n;
    char key[KEY_SIZE];
    int value;
    bool done;
    sem_t sem_w;
    sem_t sem_r;
} shared_c;

void init_shared_c(shared_c *sc) {
    int err;

    sc->done = 0;

    if ((err = sem_init(&sc->sem_w, 0, 1)) != 0)
        exit_with_err("sem_init", err);

    if ((err = sem_init(&sc->sem_r, 0, 0)) != 0)
        exit_with_err("sem_init", err);
}

void destroy_shared_c(shared_c *sc) {
    sem_destroy(&sc->sem_w);
    sem_destroy(&sc->sem_r);
    free(sc);
}

typedef struct {
    pthread_t tid;
    unsigned thread_n;
    char *db_file;
    char *query_file;

    shared_db *sdb;
    shared_c *sc;
} thread_data;

void db_thread(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;
    FILE *f;
    char buffer[BUFFER_SIZE];
    char *key;
    char *s_value;
    int value;

    // Fase di caricamento
    if ((f = fopen(td->db_file, "r")) == NULL)
        exit_with_sys_err("fopen");

    while (fgets(buffer, BUFFER_SIZE, f)) {
        if ((key = strtok(buffer, ":")) != NULL &&
            (s_value = strtok(NULL, ":")) != NULL) {

            value = atoi(s_value);

            printf("DB-IN-%u: inserisco (%s,%d)\n", td->thread_n, key, value);
            hash_table_insert(td->sdb->h, key, value);
        }
    }

    fclose(f);

    // Passaggio alla seconda fase
    printf("DB-IN-%u: sono arrivato alla barriera\n", td->thread_n);
    if ((err = pthread_barrier_wait(&td->sdb->barrier)) > 0)
        exit_with_err("pthread_barrier_wait", err);

    if ((f = fopen(td->query_file, "r")) == NULL)
        exit_with_sys_err("fopen");

    // Fase di ricerca
    while (fgets(buffer, BUFFER_SIZE, f)) {
        if (buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = '\0';
        
        if (hash_table_search(td->sdb->h, buffer, &value)) {
            printf("DB-IN-%u: elemento trovato (%s,%d)\n", td->thread_n, buffer,
                   value);
            if ((err = sem_wait(&td->sc->sem_w)) != 0)
                exit_with_err("sem_wait", err);

            td->sc->thread_n = td->thread_n;
            td->sc->value = value;
            strncpy(td->sc->key, buffer, KEY_SIZE);

            if ((err = sem_post(&td->sc->sem_r)) != 0)
                exit_with_err("sem_post", err);
        }
    }

    if ((err = sem_wait(&td->sc->sem_w)) != 0)
        exit_with_err("sem_wait", err);

    td->sc->done = 1;

    if ((err = sem_post(&td->sc->sem_r)) != 0)
        exit_with_err("sem_post", err);

    fclose(f);
}

void c_thread(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;
    unsigned long stats[td->thread_n];

    for (unsigned i = 0; i < td->thread_n; i++)
        stats[i] = 0;

    unsigned done_counter = 0;

    while (1) {
        if ((err = sem_wait(&td->sc->sem_r)) != 0)
            exit_with_err("sem_wait", err);

        if (td->sc->done) {
            td->sc->done = 0;
            done_counter++;

            if (done_counter == td->thread_n)
                break;
        } else {
            printf("C: (thread_n: %u, %s, %d)\n", td->sc->thread_n, td->sc->key,
                   td->sc->value);
            stats[td->sc->thread_n - 1] += td->sc->value;
        }

        if ((err = sem_post(&td->sc->sem_w)) != 0)
            exit_with_err("sem_post", err);
    }

    for (unsigned i = 0; i < td->thread_n; i++)
        printf("DB-IN-%u: totale %lu\n", i + 1, stats[i]);
}

int main(int argc, char **argv) {
    if (argc < 3 || (argc - 1) % 2 != 0) {
        printf("Usage: %s <db-file-1> <db-file-2> <...> <db-file-n> "
               "<query-file-1> <query-file-2> <...> <query-file-n>\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;
    unsigned nthread = (argc - 1) / 2;
    thread_data td[nthread + 1];
    shared_db *sdb = malloc(sizeof(shared_db));
    shared_c *sc = malloc(sizeof(shared_c));

    init_shared_db(sdb, nthread);
    init_shared_c(sc);

    for (int i = 0; i < nthread; i++) {
        td[i].db_file = argv[i + 1];
        td[i].query_file = argv[(argc - 1) / 2 + i + 1];
        td[i].sdb = sdb;
        td[i].sc = sc;
        td[i].thread_n = i + 1;

        if ((err = pthread_create(&td[i].tid, NULL, (void *)db_thread,
                                  &td[i])) != 0)
            exit_with_err("pthread_create", err);
    }

    td[nthread].sc = sc;
    td[nthread].thread_n = nthread;

    if ((err = pthread_create(&td[nthread].tid, NULL, (void *)c_thread,
                              &td[nthread])) != 0)
        exit_with_err("pthrea_create", err);

    for (unsigned i = 0; i <= nthread; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    destroy_shared_db(sdb);
    destroy_shared_c(sc);
}