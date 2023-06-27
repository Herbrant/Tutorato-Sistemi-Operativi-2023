#include "../lib/lib-misc.h"
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef enum { AL_N, MZ_N, PARENT_N } thread_n;

typedef struct {
    char c;
    unsigned long stats[26];
    bool done;
    sem_t sem[3];
} shared;

typedef struct {
    // dati privati
    pthread_t tid;
    char thread_n;

    // struttura dati condivisa
    shared *shared;
} thread_data;

// inizializza la struttura dati condivisa e restituisce il suo indirizzo
shared *init_shared() {
    int err;
    shared *sh = malloc(sizeof(shared));

    // inizializza il campo done a 0
    sh->done = 0;

    // inizializza l'array stats a 0
    memset(sh->stats, 0, sizeof(sh->stats));

    // inizializza il semaforo del thread padre
    if ((err = sem_init(&sh->sem[PARENT_N], 0, 1)) != 0)
        exit_with_err("sem_init", err);

    // inizializza i semafori destinati ai thread al ed mz
    if ((err = sem_init(&sh->sem[AL_N], 0, 0)) != 0)
        exit_with_err("sem_init", err);

    if ((err = sem_init(&sh->sem[MZ_N], 0, 0)) != 0)
        exit_with_err("sem_init", err);

    return sh;
}

void shared_destroy(shared *sh) {
    for (int i = 0; i < 3; i++)
        sem_destroy(&sh->sem[i]);

    free(sh);
}

void stats(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;

    while (1) {
        if ((err = sem_wait(&td->shared->sem[td->thread_n])) != 0)
            exit_with_err("sem_wait", err);

        if (td->shared->done)
            break;

        td->shared->stats[td->shared->c - 'a']++;

        if ((err = sem_post(&td->shared->sem[PARENT_N])) != 0)
            exit_with_err("sem_post", err);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <file.txt>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err, fd;
    thread_data td[2];
    shared *sh = init_shared();
    char *map;
    struct stat s_file;

    // inizializzo le strutture dati per i thread
    for (int i = 0; i < 2; i++) {
        td[i].shared = sh;
        td[i].thread_n = i;
    }

    // creazione thread

    // thread AL
    if ((err = pthread_create(&td[AL_N].tid, NULL, (void *)stats,
                              (void *)&td[AL_N])) != 0)
        exit_with_err("pthread_create", err);

    // thread MZ
    if ((err = pthread_create(&td[MZ_N].tid, NULL, (void *)stats,
                              (void *)&td[MZ_N])) != 0)
        exit_with_err("pthread_create", err);

    // apro il file
    if ((fd = open(argv[1], O_RDONLY)) == -1)
        exit_with_err("open", err);

    // eseguo lo stat del file per conoscere le sua dimensione
    if ((err = fstat(fd, &s_file)) == -1)
        exit_with_err("fstat", err);

    // mappo il file in memoria in sola lettura
    if ((map = mmap(NULL, s_file.st_size, PROT_READ, MAP_SHARED, fd, 0)) ==
        MAP_FAILED)
        exit_with_err("mmap", err);

    // chiudo il file (non più necessario dopo aver effettuato la mappatura in
    // memoria)
    if ((err = close(fd)) == -1)
        exit_with_err("close", err);

    // leggo il file carattere per carattere
    int i = 0;

    while (i < s_file.st_size) {
        if ((map[i] >= 'a' && map[i] <= 'z') ||
            (map[i] >= 'A' && map[i] <= 'Z')) {
            // aspetto che il thread lettore abbia completato
            if ((err = sem_wait(&sh->sem[PARENT_N])) != 0)
                exit_with_err("sem_wait", err);

            // inserisco il carattere nella struttura dati condivisa
            sh->c = tolower(map[i]);

            // sveglio il thread che deve gestire il carattere
            if (map[i] <= 'l') {
                if ((err = sem_post(&sh->sem[AL_N])) != 0)
                    exit_with_err("sem_post", err);
            } else {
                if ((err = sem_post(&sh->sem[MZ_N])) != 0)
                    exit_with_err("sem_post", err);
            }
        }

        i++; // incremento l'indice per l'array map
    }

    // aspetto che il thread lettore abbia completato
    if ((err = sem_wait(&sh->sem[PARENT_N])) != 0)
        exit_with_err("sem_wait", err);

    // stampo le statistiche
    printf("Statistiche sul file: %s\n", argv[1]);

    for (int i = 0; i < 26; i++)
        printf("%c: %lu\t", i + 'a', sh->stats[i]);

    printf("\n");

    // notifico ai thread la fine dei lavori
    sh->done = 1;
    if ((err = sem_post(&sh->sem[AL_N])) != 0)
        exit_with_err("sem_post", err);
    if ((err = sem_post(&sh->sem[MZ_N])) != 0)
        exit_with_err("sem_post", err);

    // attendo l'uscita dei thread
    for (int i = 0; i < 2; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    // distruggo la struttura dati condivisa
    shared_destroy(sh);

    // rilascio la mappatura del file in memoria
    if ((err = munmap(map, s_file.st_size)) == -1)
        exit_with_err("munmap", err);

    exit(EXIT_SUCCESS);
}