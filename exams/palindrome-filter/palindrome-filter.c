#include "../../lib/lib-misc.h"
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 4096

typedef enum { READER_N, PALINDROME_N, WRITER_N } thread_n;

typedef struct {
    char buffer[BUFFER_SIZE];
    bool done;
    sem_t sem[3];
} shared;

typedef struct {
    pthread_t tid;
    char *filename;
    shared *shared;
} thread_data;

void init_shared(shared *sh) {
    int err;

    sh->done = 0;

    if ((err = sem_init(&sh->sem[READER_N], 0, 1)) != 0)
        exit_with_err("sem_init", err);

    if ((err = sem_init(&sh->sem[PALINDROME_N], 0, 0)) != 0)
        exit_with_err("sem_init", err);

    if ((err = sem_init(&sh->sem[WRITER_N], 0, 0)) != 0)
        exit_with_err("sem_init", err);
}

void destroy_shared(shared *sh) {
    for (int i = 0; i < 3; i++)
        sem_destroy(&sh->sem[i]);

    free(sh);
}

void reader(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;
    char buffer[BUFFER_SIZE];

    FILE *f;

    if ((f = fopen(td->filename, "r")) == NULL)
        exit_with_sys_err("fopen");

    while (fgets(buffer, BUFFER_SIZE, f)) {
        if (buffer[strlen(buffer) - 1] == '\n')
            buffer[strlen(buffer) - 1] = '\0';

        if ((err = sem_wait(&td->shared->sem[READER_N])) != 0)
            exit_with_err("sem_wait", err);

        strncpy(td->shared->buffer, buffer, BUFFER_SIZE);

        if ((err = sem_post(&td->shared->sem[PALINDROME_N])) != 0)
            exit_with_err("sem_post", err);
    }

    if ((err = sem_wait(&td->shared->sem[READER_N])) != 0)
        exit_with_err("sem_wait", err);

    td->shared->done = 1;

    if ((err = sem_post(&td->shared->sem[PALINDROME_N])) != 0)
        exit_with_err("sem_post", err);

    if ((err = sem_post(&td->shared->sem[WRITER_N])) != 0)
        exit_with_err("sem_post", err);

    fclose(f);
}

bool is_palindrome(char *s) {
    for (int i = 0; i < strlen(s); i++)
        if (s[i] != s[strlen(s) - 1 - i])
            return 0;

    return 1;
}

void palindrome(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;

    while (1) {
        if ((err = sem_wait(&td->shared->sem[PALINDROME_N])) != 0)
            exit_with_err("sem_wait", err);

        if (td->shared->done)
            break;

        if (is_palindrome(td->shared->buffer)) {
            if ((err = sem_post(&td->shared->sem[WRITER_N])) != 0)
                exit_with_err("sem_post", err);
        } else {
            if ((err = sem_post(&td->shared->sem[READER_N])) != 0)
                exit_with_err("sem_post", err);
        }
    }
}

void writer(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;

    while (1) {
        if ((err = sem_wait(&td->shared->sem[WRITER_N])) != 0)
            exit_with_err("sem_wait", err);

        if (td->shared->done)
            break;

        printf("%s\n", td->shared->buffer);

        if ((err = sem_post(&td->shared->sem[READER_N])) != 0)
            exit_with_err("sem_post", err);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <input-file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;
    thread_data td[3];
    shared *sh = malloc(sizeof(shared));
    init_shared(sh);

    for (int i = 0; i < 3; i++)
        td[i].shared = sh;

    td[READER_N].filename = argv[1];

    if ((err = pthread_create(&td[READER_N].tid, NULL, (void *)reader,
                              &td[READER_N])) != 0)
        exit_with_err("pthread_create", err);

    if ((err = pthread_create(&td[PALINDROME_N].tid, NULL, (void *)palindrome,
                              &td[PALINDROME_N])) != 0)
        exit_with_err("pthread_create", err);

    if ((err = pthread_create(&td[WRITER_N].tid, NULL, (void *)writer,
                              &td[WRITER_N])) != 0)
        exit_with_err("pthread_create", err);

    for (int i = 0; i < 3; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    destroy_shared(sh);
}