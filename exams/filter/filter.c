#include "../../lib/lib-misc.h"
#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define BUFFER_SIZE 1024

// enum per identificare il tipo di filtro
typedef enum { TOUPPER_FILTER, TOLOWER_FILTER, REPLACE_FILTER } filter_type;

typedef struct {
    char buffer[BUFFER_SIZE];
    unsigned turn;
    unsigned nfilter;
    bool done;

    // strumenti per la mutua esclusione e la sincronizzazione
    pthread_mutex_t lock;
    pthread_cond_t *cond;
} shared;

typedef struct {
    // dati privati
    pthread_t tid;
    unsigned thread_n;
    filter_type type;
    char *filter_data1;
    char *filter_data2;

    // dati condivisi
    shared *sh;
} thread_data;

// funzione per l'inizializzazione della struttura dati condivisa
void init_shared(shared *sh, unsigned nfilter) {
    int err;

    sh->done = sh->turn = 0;
    sh->nfilter = nfilter;

    // inizializzo il mutex
    if ((err = pthread_mutex_init(&sh->lock, NULL)) != 0)
        exit_with_err("pthread_mutex_init", err);

    // alloco dinamicamente nfilter + 1 variabili condizione
    sh->cond = malloc(sizeof(pthread_cond_t) * (nfilter + 1));

    // inizializzo le variabili condizione
    for (unsigned i = 0; i < nfilter + 1; i++)
        if ((err = pthread_cond_init(&sh->cond[i], NULL)) != 0)
            exit_with_err("pthread_cond_init", err);
}

// funzione per la deallocazione della struttura dati condivisa
void destroy_shared(shared *sh) {
    // distruggo il mutex
    pthread_mutex_destroy(&sh->lock);

    // distruggo le variabili condizione
    for (unsigned i = 0; i < sh->nfilter + 1; i++)
        pthread_cond_destroy(&sh->cond[i]);

    // dealloco l'array di variabili condizione
    free(sh->cond);
    // dealloco la struttura dati condivisa
    free(sh);
}

// funzione per l'applicazione di un filtro di tipo TOUPPER
void toupper_filter(char *buffer, char *word) {
    char *s;

    // itero fin quando trovo la stringa word
    while ((s = strstr(buffer, word)) != NULL) {
        // itero sui caratteri della stringa s
        for (int i = 0; i < strlen(word); i++)
            // modifico l'i-esimo carattere della stringa, trasformandolo in
            // maiuscolo
            s[i] = toupper(s[i]);
    }
}

// funzione per l'applicazione di un filtro di tipo TOLOWER
void tolower_filter(char *buffer, char *word) {
    char *s;

    // itero fin quando trovo la stringa word
    while ((s = strstr(buffer, word)) != NULL) {
        // itero sui caratteri della stringa s
        for (int i = 0; i < strlen(word); i++)
            // modifico l'i-esimo carattere della stringa, trasformandolo in
            // minuscolo
            s[i] = tolower(s[i]);
    }
}

// funzione per l'applicazione di un filtro di tipo REPLACE
void replace_filter(char *buffer, char *word1, char *word2) {
    char *s;
    int i;
    char tmp_buffer[BUFFER_SIZE];

    // itero fin quando trovo la stringa word1
    while ((s = strstr(buffer, word1)) != NULL) {
        // faccio una copia di buffer in tmp_buffer
        strncpy(tmp_buffer, buffer, BUFFER_SIZE);

        // trovo l'indice in cui si trova il primo carattere
        // della stringa s
        i = s - buffer;
        // inserisco un carattere di terminazione in posizione i
        // (utile per eseguire una copia parziale di buffer)
        buffer[i] = '\0';

        // setto la stringa buffer come la concatenazione della sottostringa di
        // buffer (quella precedente alla word1 individuata), word2 (a
        // sostituzione di word1) e la restante parte della stringa presente in
        // buffer (successiva alla word1 individuata)
        sprintf(buffer, "%s%s%s", buffer, word2,
                tmp_buffer + i + strlen(word1));
    }
}

// funzione filter eseguita dai thread
void filter(void *arg) {
    int err;
    thread_data *td = (thread_data *)arg;

    while (1) {
        // provo ad ottenere il lock sulla struttura dati condivisa
        if ((err = pthread_mutex_lock(&td->sh->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        // rimango in attesa fin quando non valgono le condizioni di
        // operabilità, ovvero fin quando non è il mio turno
        while (td->sh->turn != td->thread_n)
            if ((err = pthread_cond_wait(&td->sh->cond[td->thread_n],
                                         &td->sh->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        // incremento il turno (riportandolo a zero nel caso il thread sia
        // l'ultimo filtro da applicare)
        td->sh->turn = (td->sh->turn + 1) % (td->sh->nfilter + 1);

        // se il campo done è settato a 1 termino l'esecuzione dopo aver
        // risvegliato il filtro successivo e rilasciato il lock
        if (td->sh->done) {
            if ((err = pthread_cond_signal(&td->sh->cond[td->sh->turn])) != 0)
                exit_with_err("pthread_cond_signal", err);

            if ((err = pthread_mutex_unlock(&td->sh->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);
            break;
        }

        // verifico il tipo di filtro da applicare sulla base del campo "type"
        // presente nella struttura dati thread_data e lo applico richiamando la
        // funzione opportuna
        switch (td->type) {
        case TOUPPER_FILTER:
            toupper_filter(td->sh->buffer, td->filter_data1);
            break;
        case TOLOWER_FILTER:
            tolower_filter(td->sh->buffer, td->filter_data1);
            break;
        case REPLACE_FILTER:
            replace_filter(td->sh->buffer, td->filter_data1, td->filter_data2);
            break;
        default:
            fprintf(stderr, "Tipo di filtro non valido: %d", td->type);
            exit(EXIT_FAILURE);
            break;
        }

        // sveglio il filtro successivo o, nel caso fosse l'ultimo filtro, il
        // main
        if ((err = pthread_cond_signal(&td->sh->cond[td->sh->turn])) != 0)
            exit_with_err("pthread_cond_signal", err);

        // rilascio il lock
        if ((err = pthread_mutex_unlock(&td->sh->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <file.txt> <filter-1> [filter-2] [...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;
    FILE *f;
    thread_data td[argc - 2];
    shared *sh = malloc(sizeof(shared));
    init_shared(sh, argc - 2);
    char *word1, *word2;
    bool first_turn = 1;
    char buffer[BUFFER_SIZE];

    // itero sui filtri passati come parametri al programma
    for (int i = 2; i < argc; i++) {
        td[i - 2].thread_n = i - 1;
        td[i - 2].sh = sh;

        // parsing del filtro
        switch (argv[i][0]) {
        case '^':
            td[i - 2].type = TOUPPER_FILTER;
            td[i - 2].filter_data1 = &argv[i][1];
            break;
        case '_':
            td[i - 2].type = TOLOWER_FILTER;
            td[i - 2].filter_data1 = &argv[i][1];
            break;
        case '%':
            td[i - 2].type = REPLACE_FILTER;

            if (!(word1 = strtok(&argv[i][1], ",")) ||
                !(word2 = strtok(NULL, ","))) {
                fprintf(stderr,
                        "Errore nel filtro %s: sintassi non supportata\n",
                        argv[i]);
                exit(EXIT_FAILURE);
            }

            td[i - 2].filter_data1 = word1;
            td[i - 2].filter_data2 = word2;
            break;
        default:
            fprintf(stderr, "Errore nel filtro %s: sintassi non supportata\n",
                    argv[i]);
            exit(EXIT_FAILURE);
        }

        // creo il thread filter
        if ((err = pthread_create(&td[i - 2].tid, NULL, (void *)filter,
                                  &td[i - 2])) != 0)
            exit_with_err("pthread_create", err);
    }

    // apro il file in sola lettura
    if ((f = fopen(argv[1], "r")) == NULL)
        exit_with_sys_err("fopen");

    // leggo il file riga per riga;
    // la lettura del file viene fatta utilizzando buffer di supporto "privato"
    // per ragioni di efficienza;
    while (fgets(buffer, BUFFER_SIZE, f)) {
        // provo ad ottenere il lock sulla struttura dati condivisa
        if ((err = pthread_mutex_lock(&sh->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        // rimango in attesa fin quando non valgono le condizioni di
        // operabilità, ovvero fin quando non è il mio turno
        while (sh->turn != 0)
            if ((err = pthread_cond_wait(&sh->cond[0], &sh->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        // soluzione dummy per non stampare alla prima iterazione
        if (!first_turn)
            printf("%s", sh->buffer);
        else
            first_turn = 0;

        // copio il contenuto di buffer nel campo buffer della struttura dati
        // condivisa
        strncpy(sh->buffer, buffer, BUFFER_SIZE);
        // incremento il turno
        sh->turn++;

        // sveglio il primo filtro
        if ((err = pthread_cond_signal(&sh->cond[sh->turn])) != 0)
            exit_with_err("pthread_cond_signal", err);

        // rilascio il lock
        if ((err = pthread_mutex_unlock(&sh->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }

    // provo ad ottenere il lock
    if ((err = pthread_mutex_lock(&sh->lock)) != 0)
        exit_with_err("pthread_mutex_lock", err);

    // rimango in attesa fin quando non valgono le condizioni di
    // operabilità, ovvero fin quando non è il mio turno
    while (sh->turn != 0)
        if ((err = pthread_cond_wait(&sh->cond[0], &sh->lock)) != 0)
            exit_with_err("pthread_cond_wait", err);

    // stampo l'ultima stringa
    printf("%s\n", sh->buffer);
    // imposto il campo done a 1 per segnalare la fine dei lavori
    sh->done = 1;
    // incremento il turno
    sh->turn++;

    // sveglio il primo filtro
    if ((err = pthread_cond_signal(&sh->cond[sh->turn])) != 0)
        exit_with_err("pthread_cond_signal", err);

    // rilascio il lock
    if ((err = pthread_mutex_unlock(&sh->lock)) != 0)
        exit_with_err("pthread_mutex_unlock", err);

    // attengo la terminazione dei thread filter
    for (int i = 0; i < argc - 2; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    // chiudo il file
    fclose(f);
    // distruggo la struttura dati condivisa
    destroy_shared(sh);
}