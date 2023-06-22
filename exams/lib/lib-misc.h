/*
 * libreria di servizio ufficiosa (in versione esami)
 */

#ifndef LIB_OSLAB_MISC_H
#define LIB_OSLAB_MISC_H

#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* esce con un exit code di errore visualizzando un prefisso a scelta `s`
 * seguito dal messaggio di errore standard associato all'attuale codice in
 * `errno` */
#define exit_with_sys_err(s)                                                   \
    do {                                                                       \
        perror((s));                                                           \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

/* esce con un exit code di errore visualizzando un prefisso a scelta `s`
 * seguito dal messaggio di errore standard associato al codice di errore
 * passato (utile per le funzione delle pthread che non usano `errno`) */
#define exit_with_err(s, e)                                                    \
    do {                                                                       \
        fprintf(stderr, "%s: %s\n", (s), strerror((e)));                       \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

/* esce con un exit code di errore visualizzando un messaggio a scelta con le
 * convenzioni del printf */
#define exit_with_err_msg(...)                                                 \
    do {                                                                       \
        fprintf(stderr, __VA_ARGS__);                                          \
        exit(EXIT_FAILURE);                                                    \
    } while (0)

/* PS: il costrutto `do {} while (0)` nella macro serve per forzare il
 * compilatore a trattarlo come una vera e propria funzione (ad esempio
 * obbligando l'uso del punto-e-virgola subito dopo) */

#endif /* LIB_OSLAB_MISC_H */