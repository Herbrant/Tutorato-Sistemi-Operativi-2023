#include "../../lib/lib-misc.h"
#include "list-thread-unsafe.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUFFER_SIZE 4096
#define SLEEP_S 8

typedef struct {
    pthread_rwlock_t lock; // lock in lettura/scrittura
    bool done;
    list *l; // puntatore alla lista condivisa
} shared;

typedef struct {
    // dati privati
    pthread_t tid;
    unsigned thread_n;
    char *filename;

    // dati condivisi
    shared *shared;
} thread_data;

void init_shared(shared *sh) {
    int err;

    // inizializzo il lock
    if ((err = pthread_rwlock_init(&sh->lock, NULL)) != 0)
        exit_with_err("pthread_rwlock_init", err);

    // alloco la struttura dati per la lista
    sh->l = malloc(sizeof(list));

    sh->done = 0;
    init_list(sh->l);
}

void destroy_shared(shared *sh) {
    pthread_rwlock_destroy(&sh->lock);
    list_destroy(sh->l);
    free(sh);
}

void reader(void *arg) {
    thread_data *td = (thread_data *)arg;
    int err;
    FILE *f;
    char buffer[BUFFER_SIZE];
    char *key, *s_value;
    int value;

    // apro il file in sola lettura
    if ((f = fopen(td->filename, "r")) == NULL)
        exit_with_sys_err("fopen");

    // leggo il file riga per riga utilizzando un buffer
    while ((fgets(buffer, BUFFER_SIZE, f))) {
        if ((key = strtok(buffer, ":")) != NULL &&
            (s_value = strtok(NULL, ":")) != NULL) {

            value = atoi(s_value);

            // provo ad ottenere il lock in scrittura
            if ((err = pthread_rwlock_wrlock(&td->shared->lock)) != 0)
                exit_with_err("pthread_rwlock_wrlock", err);

            if (td->shared->done) {
                printf("R%d: esco.\n", td->thread_n);
                // rilascio il lock sulla struttura dati condivisa ed esco
                if ((err = pthread_rwlock_unlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_rwlock_unlock", err);

                break;
            }

            // inserisco l'elemento all'interno della lista
            list_insert(td->shared->l, key, value);

            // rilascio il lock sulla struttura dati condivisa
            if ((err = pthread_rwlock_unlock(&td->shared->lock)) != 0)
                exit_with_err("pthread_rwlock_unlock", err);

            printf("R%d: inserito l'elemento (%s,%d)\n", td->thread_n, key,
                   value);

            sleep(SLEEP_S); // rimango in attesa per SLEEP_S secondi
        }
    }

    fclose(f);
}

void query(void *arg) {
    thread_data *td = (thread_data *)arg;
    int err;
    char query[BUFFER_SIZE];
    int result;
    bool ret_value;

    while (1) {
        // leggo la query dallo standard input
        if (fgets(query, BUFFER_SIZE, stdin)) {
            if (query[strlen(query) - 1] == '\n')
                query[strlen(query) - 1] = '\0';

            // verifico se l'utente ha chiesto di terminare
            if (!strcmp(query, "quit")) {
                if ((err = pthread_rwlock_wrlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_rwlock_wrlock", err);

                td->shared->done = 1;

                if ((err = pthread_rwlock_unlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_rwlock_unlock", err);

                printf("Q: chiusura dei thread...\n");
                break;
            } else {
                // provo a ottenere il lock lettura
                if ((err = pthread_rwlock_rdlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_rwlock_rdlock", err);

                // effettuo una ricerca all'interno della lista
                ret_value = list_search(td->shared->l, query, &result);

                // rilascio il lock sulla struttura dati condivisa
                if ((err = pthread_rwlock_unlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_rwlock_unlock", err);

                if (ret_value)
                    printf("Q: occorrenza trovata (%s,%d)\n", query, result);
                else
                    printf("Q: non è stata trovata alcuna occorrenza con "
                           "chiave %s\n",
                           query);
            }
        }
    }
}

void counter(void *arg) {
    thread_data *td = (thread_data *)arg;
    int err, counter;

    while (1) {
        if ((err = pthread_rwlock_rdlock(&td->shared->lock)) != 0)
            exit_with_err("pthread_rwlock_rdlock", err);

        if (td->shared->done) {
            // rilascio il lock sulla struttura dati condivisa ed esco
            if ((err = pthread_rwlock_unlock(&td->shared->lock)) != 0)
                exit_with_err("pthread_rwlock_unlock", err);

            printf("C: esco.\n");
            break;
        }

        counter = list_count(td->shared->l);

        if ((err = pthread_rwlock_unlock(&td->shared->lock)) != 0)
            exit_with_err("pthread_rwlock_unlock", err);

        printf("C: sono presenti %d elementi all'interno della lista\n",
               counter);

        sleep(SLEEP_S);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <db-file1> <db-file2> <...> <db-filen>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;

    // creo un array di thread_data, uno per ciascun thread (n+2)
    thread_data td[argc + 1];
    shared *sh = malloc(sizeof(shared));
    init_shared(sh);

    // creo i thread reader
    for (int i = 0; i < argc - 1; i++) {
        td[i].shared = sh;
        td[i].filename = argv[i + 1];
        td[i].thread_n = i + 1;

        if ((err = pthread_create(&td[i].tid, NULL, (void *)reader, &td[i])) !=
            0)
            exit_with_err("pthread_create", err);
    }

    // creo il thread Q
    td[argc - 1].shared = sh;
    if ((err = pthread_create(&td[argc - 1].tid, NULL, (void *)query,
                              &td[argc - 1])) != 0)
        exit_with_err("pthread_create", err);

    // creo il thread C
    td[argc].shared = sh;
    if ((err = pthread_create(&td[argc].tid, NULL, (void *)counter,
                              &td[argc])) != 0)
        exit_with_err("pthread_create", err);

    for (int i = 0; i < argc + 1; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    destroy_shared(sh);
}