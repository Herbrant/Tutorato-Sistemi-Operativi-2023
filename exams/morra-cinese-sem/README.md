# Morra Cinese (semafori)
Creare un programma **morra-cinese.c** in linguaggio C che accetti invocazioni sulla riga di comando del tipo:
```bash
morra-cinese <numero-partite>
```

Il programma gestisce una serie di partite tra due giocatori virtuali (thread) P1 e P2 che giocano alla Morra Cinese. Il programma creerà: due thread P1 e P2 che rappresenteranno i giocatori, un thread giudice G e un thread tabellone T. I thread condivideranno una struttura dati e useranno un certo numero di semafori (minimo a scelta dello studente) da usare opportunamente.

La struttura dati dovrà contenere i dati relativi ad una singola partita con i seguenti specifici campi:
- **mossa_p1**: C(arta)/F(orbici)/S(asso)
- **mossa_p2**: C/arta/F(orbici)/S(asso)
- **vincitore**: 1 (giocatore 1)/2 giocatore(2)
- (eventuali dati ausiliari)

Iniziata una partita, P1 e P2 popolano i relativi campi con una propria mossa casuale; ognuno, fatta la mossa, deve segnalare la cosa al processo G che valuterà chi ha vinto, se c'è un vincitore allora G popolerà il campo apposito e segnalerà la disponibilità di un nuovo esito al processo T; se invece la partita è patta (stessa mossa), allora lancerà direttamente una nuova partita. Il processo T, in caso di vittoria, aggiornerà la propria classifica interna e avvierà, se necessario, una nuova partita. Sempre T, alla fine della serie di partite, decreterà l'eventuale vincitore.

I thread dovranno tutti terminare spontaneamente alla fine del torneo.

