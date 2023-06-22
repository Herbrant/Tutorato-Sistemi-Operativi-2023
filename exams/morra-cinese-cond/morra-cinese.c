#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define TO_MOVE 3

typedef enum { PLAYER1_N, PLAYER2_N, JUDGE_N, SCOREBOARD_N } thread_n;
typedef enum { CARTA, FORBICE, SASSO } mossa;
char *nome_mosse[3] = {"carta", "forbice", "sasso"};

#define exit_with_err(s, e)                                                    \
    do {                                                                       \
        fprintf(stderr, "%s: %s\n", (s), strerror((e)));                       \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

typedef struct {
    mossa mosse[2];
    char vincitore;
    bool done;
    unsigned n_matches;
    pthread_mutex_t lock;
    pthread_cond_t pcond[4];
} match;

typedef struct {
    pthread_t tid;
    thread_n thread_n;
    match *match;
} thread_data;

void player(void *arg) {
    int err;
    char mossa;
    thread_data *td = (thread_data *)(arg);

    while (1) {
        if ((err = pthread_mutex_lock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        while (td->match->mosse[td->thread_n] != TO_MOVE && !td->match->done)
            if ((err = pthread_cond_wait(&td->match->pcond[td->thread_n],
                                         &td->match->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        if (td->match->done) {
            if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);
            break;
        }

        td->match->mosse[td->thread_n] = rand() % 3;

        printf("P%d: mossa '%s'\n", td->thread_n + 1,
               nome_mosse[td->match->mosse[td->thread_n]]);

        if ((err = pthread_cond_signal(&td->match->pcond[JUDGE_N])) != 0)
            exit_with_err("pthread_cond_signal", err);

        if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }

    pthread_exit(NULL);
}

char whowins(mossa *mosse) {
    if (mosse[0] == mosse[1])
        return 0;

    if ((mosse[0] == CARTA && mosse[1] == SASSO) ||
        (mosse[0] == FORBICE && mosse[1] == CARTA) ||
        (mosse[0] == SASSO && mosse[1] == FORBICE))
        return 1;

    return 2;
}

void judge(void *arg) {
    int err;
    char winner;
    thread_data *td = (thread_data *)(arg);
    unsigned completed_matches = 0;

    while (1) {
        if ((err = pthread_mutex_lock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        while ((td->match->mosse[PLAYER1_N] == TO_MOVE ||
                td->match->mosse[PLAYER2_N] == TO_MOVE) &&
               !td->match->done)
            if ((err = pthread_cond_wait(&td->match->pcond[JUDGE_N],
                                         &td->match->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        if (td->match->done) {
            if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
                exit_with_err("pthread_mutex_unlock", err);
            break;
        }

        printf("G: mossa P1: %s\t", nome_mosse[td->match->mosse[PLAYER1_N]]);
        printf("mossa P2: %s\n", nome_mosse[td->match->mosse[PLAYER2_N]]);

        winner = whowins(td->match->mosse);

        td->match->mosse[PLAYER1_N] = TO_MOVE;
        td->match->mosse[PLAYER2_N] = TO_MOVE;

        if (!winner) {
            if ((err = pthread_cond_signal(&td->match->pcond[PLAYER1_N])) != 0)
                exit_with_err("pthread_cond_signal", err);
            if ((err = pthread_cond_signal(&td->match->pcond[PLAYER2_N])) != 0)
                exit_with_err("pthread_cond_signal", err);
        } else {
            completed_matches++;
            td->match->vincitore = winner;
            printf("G: partita n.%d vinta da P%d\n", completed_matches, winner);
            if ((err = pthread_cond_signal(&td->match->pcond[SCOREBOARD_N])) !=
                0)
                exit_with_err("pthread_cond_signal", err);
        }

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
        if ((err = pthread_mutex_lock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_lock", err);

        while (td->match->vincitore == 0)
            if ((err = pthread_cond_wait(&td->match->pcond[SCOREBOARD_N],
                                         &td->match->lock)) != 0)
                exit_with_err("pthread_cond_wait", err);

        scores[td->match->vincitore - 1]++;

        if (i < td->match->n_matches - 1)
            printf("T: classifica temporanea: P1:%d P2:%d\n\n", scores[0],
                   scores[1]);

        td->match->vincitore = 0;

        if ((err = pthread_cond_signal(&td->match->pcond[PLAYER1_N])) != 0)
            exit_with_err("pthread_cond_wait", err);
        if ((err = pthread_cond_signal(&td->match->pcond[PLAYER2_N])) != 0)
            exit_with_err("pthread_cond_wait", err);
        if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
            exit_with_err("pthread_mutex_unlock", err);
    }

    pthread_mutex_lock(&td->match->lock);
    printf("T: classifica finale: %d %d\n", scores[0], scores[1]);
    printf("T: vincitore del torneo: P%d\n", scores[0] > scores[1] ? 1 : 2);
    td->match->done = 1;

    for (int i = 0; i < 3; i++)
        if ((err = pthread_cond_signal(&td->match->pcond[i])) != 0)
            exit_with_err("pthread_cond_signal", err);

    if ((err = pthread_mutex_unlock(&td->match->lock)) != 0)
        exit_with_err("pthread_mutex_unlock", err);

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
    match *m = malloc(sizeof(match));
    m->n_matches = n_matches;
    m->mosse[PLAYER1_N] = m->mosse[PLAYER2_N] = TO_MOVE;
    m->vincitore = 0;

    if ((err = pthread_mutex_init(&m->lock, NULL)) != 0)
        exit_with_err("pthread_mutex_init", err);

    for (int i = 0; i < 4; i++)
        if ((err = pthread_cond_init(&m->pcond[i], NULL)) != 0)
            exit_with_err("pthread_cond_init", err);

    for (int i = 0; i < 4; i++) {
        td[i].thread_n = i;
        td[i].match = m;
    }

    srand(time(NULL));

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

    // Pthread join
    for (int i = 0; i < 4; i++)
        if ((err = pthread_join(td[i].tid, NULL)) != 0)
            exit_with_err("pthread_join", err);

    pthread_mutex_destroy(&m->lock);

    for (int i = 0; i < 4; i++)
        pthread_cond_destroy(m->pcond);

    free(m);
    exit(EXIT_SUCCESS);
}