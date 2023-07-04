#include "../../lib/lib-misc.h"
#include "hash-table-thread-safe.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BUFFER_SIZE 4096
#define SLEEP_S 8

typedef struct {
    bool done;
    hash_table *h;        // puntatore all'hash-table condivisa
    pthread_mutex_t lock; // lock per il campo done
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

    // inizializzo il mutex
    if ((err = pthread_mutex_init(&sh->lock, NULL)) != 0)
        exit_with_err("pthread_mutex_init", err);

    sh->done = 0;
    sh->h = new_hash_table(1024);
}

void destroy_shared(shared *sh) {
    hash_table_destroy(sh->h);
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
            if ((err = pthread_mutex_lock(&td->shared->lock)) != 0)
                exit_with_err("pthread_mutex_lock", err);

            if (td->shared->done) {
                printf("R%d: esco.\n", td->thread_n);
                // rilascio il lock sulla struttura dati condivisa ed esco
                if ((err = pthread_mutex_unlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_mutex_unlock", err);

                break;
            }

            // rilascio il lock sulla struttura dati condivisa
            if ((err = pthread_mutex_unlock(&td->shared->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);

            // inserisco l'elemento all'interno dell'hash table
            hash_table_insert(td->shared->h, key, value);

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
                if ((err = pthread_mutex_lock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_mutex_lock", err);

                td->shared->done = 1;

                if ((err = pthread_mutex_unlock(&td->shared->lock)) != 0)
                    exit_with_err("pthread_mutex_unlock", err);

                printf("Q: chiusura dei thread...\n");
                break;
            } else {
                // effettuo una ricerca all'interno della lista
                ret_value = hash_table_search(td->shared->h, query, &result);

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

        if ((err = pthread_mutex_lock(&td->shared->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        if (td->shared->done) {
            // rilascio il lock sulla struttura dati condivisa ed esco
            if ((err = pthread_mutex_unlock(&td->shared->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);

            printf("C: esco.\n");
            break;
        }

        if ((err = pthread_mutex_unlock(&td->shared->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);

        counter = hash_table_get_n(td->shared->h);

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