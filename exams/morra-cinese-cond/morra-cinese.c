#include "../../lib/lib-misc.h"
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#define TO_MOVE 3

typedef enum { PLAYER1_N, PLAYER2_N, JUDGE_N, SCOREBOARD_N } thread_n;
typedef enum { CARTA, FORBICE, SASSO } mossa;
char *nome_mosse[3] = {"carta", "forbice", "sasso"};

typedef struct {
    mossa mosse[2];     // mosse dei giocatori per la partita corrente
    char vincitore;     // vincitore della partita corrente
    bool done;          // flag per indicare la fine del torneo
    unsigned n_matches; // numero di partite da effettuare per il torneo
    // mutex per gestire l'accesso con mutua esclusione alla struttura dati
    // convisa
    pthread_mutex_t lock;

    // variabili conidizone necessarie per gestire la sincronizzazione senza
    // spin lock
    pthread_cond_t pcond[4];
} match;

typedef struct {
    // Variabili private
    pthread_t tid;
    thread_n thread_n;

    // Struttura dati condivisa
    match *match;
} thread_data;

void player(void *arg) {
    int err;
    char mossa;
    thread_data *td = (thread_data *)(arg);

    while (1) {
        // provo ad ottenere il lock alla struttura dati condivisa
        if ((err = pthread_mutex_lock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        // verifico se valgono le condizioni di operabilità
        while (td->match->mosse[td->thread_n] != TO_MOVE && !td->match->done)
            // rimango in attesa nel caso in cui queste non valgono utilizzando
            // la variabile condizione rilasciando il implicitamente il mutex
            if ((err = pthread_cond_wait(&td->match->pcond[td->thread_n],
                                         &td->match->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        // se il torneo è finito rilascio il lock ed esco
        if (td->match->done) {
            if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);
            break;
        }

        // genero una mossa casuale e la inserisco all'interno dell'array delle
        // mosse utilizzando, come indice, il campo thread_n (0 per il player 1,
        // 1 per il player 2)
        td->match->mosse[td->thread_n] = rand() % 3;

        printf("P%d: mossa '%s'\n", td->thread_n + 1,
               nome_mosse[td->match->mosse[td->thread_n]]);

        // sveglio il thread giudice affinché lui possa verificare le sue
        // condizioni di operabilità
        if ((err = pthread_cond_signal(&td->match->pcond[JUDGE_N])) != 0)
            exit_with_err("pthread_cond_signal", err);

        // rilascio il lock
        if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }

    pthread_exit(NULL);
}

// funzione che determina il vincitore date le due mosse (restituisce 0 nel caso
// di patta)
char whowins(mossa *mosse) {
    if (mosse[PLAYER1_N] == mosse[PLAYER2_N])
        return 0;

    if ((mosse[PLAYER1_N] == CARTA && mosse[PLAYER2_N] == SASSO) ||
        (mosse[PLAYER1_N] == FORBICE && mosse[PLAYER2_N] == CARTA) ||
        (mosse[PLAYER1_N] == SASSO && mosse[PLAYER2_N] == FORBICE))
        return 1;

    return 2;
}

void judge(void *arg) {
    int err;
    char winner;
    thread_data *td = (thread_data *)(arg);
    unsigned completed_matches = 0;

    while (1) {
        // provo ad ottenere il lock alla struttura dati condivisa
        if ((err = pthread_mutex_lock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        // verifico se valgono le condizioni di operabilità
        while ((td->match->mosse[PLAYER1_N] == TO_MOVE ||
                td->match->mosse[PLAYER2_N] == TO_MOVE) &&
               !td->match->done)
            // rimango in attesa nel caso in cui queste non valgono utilizzando
            // la variabile condizione rilasciando implicitamente il mutex
            if ((err = pthread_cond_wait(&td->match->pcond[JUDGE_N],
                                         &td->match->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        // se il torneo è finito rilascio il lock ed esco
        if (td->match->done) {
            if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);
            break;
        }

        printf("G: mossa P1: %s\t", nome_mosse[td->match->mosse[PLAYER1_N]]);
        printf("mossa P2: %s\n", nome_mosse[td->match->mosse[PLAYER2_N]]);

        winner = whowins(td->match->mosse);

        // imposto le mosse a TO_MOVE prima delle signal per evitare uno
        // situazione di stallo
        td->match->mosse[PLAYER1_N] = TO_MOVE;
        td->match->mosse[PLAYER2_N] = TO_MOVE;

        if (!winner) { // nel caso di patta risveglio i due player per rigiocare
                       // la partita
            if ((err = pthread_cond_signal(&td->match->pcond[PLAYER1_N])) != 0)
                exit_with_err("pthread_cond_signal", err);
            if ((err = pthread_cond_signal(&td->match->pcond[PLAYER2_N])) != 0)
                exit_with_err("pthread_cond_signal", err);
        } else { // nel caso ci sia un vincitore, sveglio il thread tabellone
                 // dopo aver impostato il vincitore nella struttura dati
                 // condivisa
            completed_matches++;
            td->match->vincitore = winner;
            printf("G: partita n.%d vinta da P%d\n", completed_matches, winner);
            if ((err = pthread_cond_signal(&td->match->pcond[SCOREBOARD_N])) !=
                0)
                exit_with_err("pthread_cond_signal", err);
        }

        // rilascio il lock
        if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }

    pthread_exit(NULL);
}

void scoreboard(void *arg) {
    int err;
    thread_data *td = (thread_data *)(arg);
    unsigned scores[2] = {0, 0};

    for (unsigned i = 0; i < td->match->n_matches; i++) {
        // provo ad ottenere il lock alla struttura dati condivisa
        if ((err = pthread_mutex_lock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        // verifico se valgono le condizioni di operabilità
        while (td->match->vincitore == 0)
            // rimango in attesa nel caso in cui queste non valgono utilizzando
            // la variabile condizione rilasciando implicitamente il mutex
            if ((err = pthread_cond_wait(&td->match->pcond[SCOREBOARD_N],
                                         &td->match->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        // incremento il punteggio del vincitore
        scores[td->match->vincitore - 1]++;

        if (i < td->match->n_matches - 1) {
            printf("T: classifica temporanea: P1:%d P2:%d\n\n", scores[0],
                   scores[1]);

            // imposto il campo vincitore nella struttura dati condivisa per
            // effettuare correttamente i controlli sulle condizioni di
            // operabilità
            td->match->vincitore = 0;
        } else { // ultima partita (fine del torneo)
            printf("T: classifica finale: %d %d\n", scores[0], scores[1]);

            if (scores[0] == scores[1])
                printf("T: il torneo è finito in parità.\n");
            else
                printf("T: vincitore del torneo: P%d\n",
                       scores[0] > scores[1] ? 1 : 2);

            // imposto il campo done a 1 per segnalare agli
            // altri thread che il torneo è finito
            td->match->done = 1;

            // sveglio il giudice in modo che possa terminare
            if ((err = pthread_cond_signal(&td->match->pcond[JUDGE_N])) != 0)
                exit_with_err("pthread_cond_singal", err);
        }

        // sveglio i player indipendentemente dal fatto che questi debbano
        // terminare o giocare una nuova partita
        if ((err = pthread_cond_signal(&td->match->pcond[PLAYER1_N])) != 0)
            exit_with_err("pthread_cond_wait", err);
        if ((err = pthread_cond_signal(&td->match->pcond[PLAYER2_N])) != 0)
            exit_with_err("pthread_cond_wait", err);

        // rilascio il lock
        if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage %s <numero-partite>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int err;
    unsigned short n_matches = atoi(argv[1]);

    if (n_matches == 0) {
        printf("Usage %s <numero-partite>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    thread_data td[4];

    // alloco dinamicamente la struttura dati condivisa
    match *m = malloc(sizeof(match));

    m->n_matches = n_matches;

    // imposto le mosse dei player a TO_MOVE in modo che questi possano
    // sbloccarsi alla prima partita
    m->mosse[PLAYER1_N] = m->mosse[PLAYER2_N] = TO_MOVE;

    // imposto il vincitore a zero per fare in modo che il thread tabellone si
    // blocchi all'avvio
    m->vincitore = 0;

    // inizializzazione del mutex e delle variabili condizione
    if ((err = pthread_mutex_init(&m->lock, NULL)) != 0)
        exit_with_err("pthread_mutex_init", err);

    for (int i = 0; i < 4; i++)
        if ((err = pthread_cond_init(&m->pcond[i], NULL)) != 0)
            exit_with_err("pthread_cond_init", err);

    // inizializzazione dei dati destinati ai thread
    for (int i = 0; i < 4; i++) {
        td[i].thread_n = i;
        td[i].match = m;
    }

    srand(time(NULL)); // imposto il seed per tutti i thread

    // Creazione dei thread
    // Player 1
    err = pthread_create(&td[PLAYER1_N].tid, NULL, (void *)player,
                         (void *)&td[PLAYER1_N]);
    if (err)
        exit_with_err("pthread_create", err);

    // Player 2
    err = pthread_create(&td[PLAYER2_N].tid, NULL, (void *)player,
                         (void *)&td[PLAYER2_N]);
    if (err)
        exit_with_err("pthread_create", err);

    // Judge
    err = pthread_create(&td[JUDGE_N].tid, NULL, (void *)judge,
                         (void *)&td[JUDGE_N]);
    if (err)
        exit_with_err("pthread_create", err);

    // Scoreboard
    err = pthread_create(&td[SCOREBOARD_N].tid, NULL, (void *)scoreboard,
                         (void *)&td[SCOREBOARD_N]);
    if (err)
        exit_with_err("pthread_create", err);

    // Pthread join: attendo che tutti i thread terminino
    for (int i = 0; i < 4; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    // Distruggo le strutture dati utilizzate
    pthread_mutex_destroy(&m->lock);

    for (int i = 0; i < 4; i++)
        pthread_cond_destroy(m->pcond);

    free(m);
    exit(EXIT_SUCCESS);
}