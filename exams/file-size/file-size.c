#include "../../lib/lib-misc.h"
#include <dirent.h>
#include <linux/limits.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#define QUEUE_CAPACITY 10

// struttura dati condivisa per il main thread e il thread stat
typedef struct {
    char pathfile[PATH_MAX];
    unsigned long size;
    bool done;

    // strumenti per la sincronizzazione
    sem_t sem_r, sem_w;
} stat_pair;

// struttura dati condivisa per i thread dir e il thread stat
typedef struct {
    char pathfiles[QUEUE_CAPACITY][PATH_MAX];
    char index_in, index_out; // indici per inserimento ed estrazione dei dati
    unsigned long done;       // campo per segnalare la fine dei lavori
    unsigned long size;       // numero di elementi presenti nella coda

    // strumenti per la sincronizzazione e la mutua esclusione
    pthread_mutex_t lock;
    sem_t full, empty;
} shared;

typedef struct {
    // dati privati
    pthread_t tid;
    unsigned long thread_n;
    char *pathdir; // path della directory da analizzare

    // dati condivisi
    shared *sh;
    stat_pair *sp;
} thread_data;

// funzione di inizializzazione della struttura dati shared
void init_shared(shared *sh) {
    int err;

    sh->index_in = sh->index_out = sh->done = sh->size = 0;

    // inizializzo il mutex
    if ((err = pthread_mutex_init(&sh->lock, NULL)) != 0)
        exit_with_err("pthread_mutex_init", err);

    // inizializzo il semaforo empty alla dimensione della coda (la coda è
    // vuota)
    if ((err = sem_init(&sh->empty, 0, QUEUE_CAPACITY)) != 0)
        exit_with_err("sem_init", err);

    // inizializzo il semaforo full a 0 (la coda è vuota)
    if ((err = sem_init(&sh->full, 0, 0)) != 0)
        exit_with_err("sem_init", err);
}

// funzione per la deallocazione della struttura dati shared
void destroy_shared(shared *sh) {
    pthread_mutex_destroy(&sh->lock);
    sem_destroy(&sh->empty);
    sem_destroy(&sh->full);
    free(sh);
}

// funzione di inizializzazione della struttura dati stat_pair
void init_stat_pair(stat_pair *sp) {
    int err;

    sp->done = 0;

    // inizializzo il semaforo sem_w, destinato allo scrittore, a 1;
    // in questo modo, alla prima iterazione il thread stat potrà scrivere sulla
    // struttura dati stat_pair
    if ((err = sem_init(&sp->sem_w, 0, 1)) != 0)
        exit_with_err("sem_init", err);

    // inizializzo il semaforo sem_r, destinato al lettore, a 0;
    // in questo modo, alla prima iterazione il main thread rimarrà in attesa
    // del thread stat
    if ((err = sem_init(&sp->sem_r, 0, 0)) != 0)
        exit_with_err("sem_init", err);
}

// funzione per la deallocazione della struttura dati stat_pair
void destroy_stat_pair(stat_pair *sp) {
    sem_destroy(&sp->sem_r);
    sem_destroy(&sp->sem_w);
    free(sp);
}

void dir_thread(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char pathfile[PATH_MAX];

    // apro la directory destinata al thread
    if ((dp = opendir(td->pathdir)) == NULL)
        exit_with_sys_err("opendir");

    printf("[D-%lu] scansione della cartella '%s'\n", td->thread_n,
           td->pathdir);

    // itero sugli oggetti del filesystem presenti nella directory
    while ((entry = readdir(dp))) {
        // concateno il path della directory con il nome del file
        snprintf(pathfile, PATH_MAX, "%s/%s", td->pathdir, entry->d_name);

        // effettuo uno stat del file
        if (lstat(pathfile, &statbuf) == -1)
            exit_with_sys_err(entry->d_name);

        // verifico se il file considerato è regolare
        if (S_ISREG(statbuf.st_mode)) {
            printf("[D-%lu] trovato il file '%s' in %s\n", td->thread_n,
                   entry->d_name, td->pathdir);

            // verifico se uno slot della coda risulta libero
            if ((err = sem_wait(&td->sh->empty)) != 0)
                exit_with_err("sem_wait", err);

            // acquisisco il lock sulla struttura dati condivisa
            if ((err = pthread_mutex_lock(&td->sh->lock)) != 0)
                exit_with_err("pthread_mutex_lock", err);

            // aggiorno l'indice dell'ultimo elemento inserito
            td->sh->index_in = (td->sh->index_in + 1) % QUEUE_CAPACITY;
            // incremento il numero di elementi presenti nella coda
            td->sh->size++;

            // aggiungo all'interno della struttura dati condivisa il nuovo
            // pathfile
            strncpy(td->sh->pathfiles[td->sh->index_in], pathfile, PATH_MAX);

            // rilascio il lock sulla struttura dati condivisa
            if ((err = pthread_mutex_unlock(&td->sh->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);

            // effettuo una signal sul semaforo full (il numero di elementi
            // presenti nella coda è aumentato)
            if ((err = sem_post(&td->sh->full)) != 0)
                exit_with_err("sem_post", err);
        }
    }

    // acquisisco il lock sulla struttura dati condivisa
    if ((err = pthread_mutex_lock(&td->sh->lock)) != 0)
        exit_with_err("pthread_mutex_lock", err);

    // incremento il numero di thread che hanno terminato i lavori
    td->sh->done++;

    // rilascio il lock sulla struttura dati condivisa
    if ((err = pthread_mutex_unlock(&td->sh->lock)) != 0)
        exit_with_err("pthread_mutex_unlock", err);

    // chiudo la directory
    closedir(dp);
}

void stat_thread(void *arg) {
    int err, sval;
    thread_data *td = (thread_data *)arg;
    char *filepath;
    struct stat statbuf;
    bool done = 0;

    // itero fino alla fine dei lavori
    while (!done) {
        // rimango in attesa fin quando la coda non contiene almeno un elemento
        if ((err = sem_wait(&td->sh->full)) != 0)
            exit_with_err("sem_wait", err);

        // rimango in wait sul semaforo sem_w per la struttura dati stat_pair
        if ((err = sem_wait(&td->sp->sem_w)) != 0)
            exit_with_err("sem_wait", err);

        // acquisisco il lock sulla struttura dati shared
        if ((err = pthread_mutex_lock(&td->sh->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        // aggiorno l'indice dell'ultimo elemento estratto
        td->sh->index_out = (td->sh->index_out + 1) % QUEUE_CAPACITY;
        // decremento il numero di elementi presenti nella coda
        td->sh->size--;

        // verifico le condizioni di terminazione (tutti i thread hanno
        // terminato e la coda è vuota)
        if (td->sh->done == (td->thread_n - 1) && td->sh->size == 0) {
            td->sp->done = 1;
            done = 1;
        }

        // estraggo l'elemento dalla coda
        filepath = td->sh->pathfiles[td->sh->index_out];

        // eseguo lo stat del file
        if (lstat(filepath, &statbuf) == -1)
            exit_with_sys_err(filepath);

        printf("[STAT] il file '%s' ha dimensione %lu byte.\n", filepath,
               statbuf.st_size);

        // inserisco i dati all'interno della struttura dati stat_pair condivisa
        // con il main thread
        strncpy(td->sp->pathfile, filepath, PATH_MAX);
        td->sp->size = statbuf.st_size;

        // rilascio il lock sulla struttura dati shared
        if ((err = pthread_mutex_unlock(&td->sh->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);

        // incremento il semaforo sem_r per notificare al main thread la
        // presenza di una nuova coppia di dati
        if ((err = sem_post(&td->sp->sem_r)) != 0)
            exit_with_err("sem_post", err);

        // incremento il semaforo empty (uno slot della coda è stato liberato)
        if ((err = sem_post(&td->sh->empty)) != 0)
            exit_with_err("sem_post", err);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <dir-1> <dir-2> ... <dir-n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;
    thread_data td[argc];
    shared *sh = malloc(sizeof(shared));
    stat_pair *sp = malloc(sizeof(stat_pair));
    init_shared(sh);
    init_stat_pair(sp);
    unsigned long total_bytes = 0;

    for (unsigned i = 0; i < argc - 1; i++) {
        td[i].pathdir = argv[i + 1];
        td[i].thread_n = i + 1;
        td[i].sh = sh;

        if ((err = pthread_create(&td[i].tid, NULL, (void *)dir_thread,
                                  &td[i])) != 0)
            exit_with_err("pthread_create", err);
    }

    td[argc - 1].sh = sh;
    td[argc - 1].sp = sp;
    td[argc - 1].thread_n = argc;

    if ((err = pthread_create(&td[argc - 1].tid, NULL, (void *)stat_thread,
                              &td[argc - 1])) != 0)
        exit_with_err("pthread_create", err);

    while (1) {
        // rimango in attesa di una coppia di dati
        if ((err = sem_wait(&sp->sem_r)) != 0)
            exit_with_err("sem_wait", err);

        // aggiorno le statistiche
        total_bytes += sp->size;

        // verifico se vale la condizione di terminazione
        if (sp->done)
            break;
        else
            printf("[MAIN] con il file %s il totale parziale è di %lu\n",
                   sp->pathfile, total_bytes);

        // incremento il semaforo sem_w per notificare al thread stat che ho
        // finito di leggere
        if ((err = sem_post(&sp->sem_w)) != 0)
            exit_with_err("sem_post", err);
    }

    printf("[MAIN] il totale finale è di %lu byte.\n", total_bytes);

    // rimango in attesa fin quando tutti i thread non sono terminati
    for (unsigned i = 0; i < argc; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    // distruggo le strutture dati allocate dinamicamente
    destroy_shared(sh);
    destroy_stat_pair(sp);
}