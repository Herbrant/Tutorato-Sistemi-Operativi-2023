# file-size-2
Creare un programma **file-size-2.c** in linguaggio C che accetti invocazioni sulla riga di
comando del tipo:

```
file-size-2 <dir-1> <dir-2> ... <dir-n>
```
Il programma dovrà determinare la dimensione totale in byte dei file regolari direttamente contenuti all'interno delle cartelle indicate (senza ricorsione).

Al suo avvio il programma creerà *n+2* thread:
- _n_ thread DIR-i che si occuperanno di scansionare la cartella assegnata alla ricerca
    di file regolari direttamente contenuti in essa (no ricorsione); per ogni file
    incontrato ne determinerà la dimensione in byte e inserirà il numero in una
    struttura dati number_set condivisa;
- due thread ADD-j il cui compito sarà quello di, in presenza di almeno due
    numeri in number_set, estrarre il minimo e il massimo attuale e di re-inserire la loro somma.

La struttura dati number_set può essere implementata in qualunque modo purché sia dinamica e non abbia una capienza massima prestabilita. Tutti i thread creati agiranno in parallelo e si coordineranno tramite mutex e variabili condizione POSIX: la scelta, il numero (minimo) e l'impiego è da determinare da parte dello studente.

I thread DIR-i termineranno quando avranno finito la scansione; i thread ADD-j
continueranno a lavorare, bloccandosi in assenza di elementi, fintanto che i thread DIR-i sono attivi e fintanto ci sono elementi potenziali da sommare; quando i thread secondari avranno finito, il thread principale MAIN ritroverà in number_set un unico elemento che corrisponderà all'occupazione totale in byte dei file trovati.

Si chiede di rispettare la struttura dell'output riportato nell'esempio a seguire.

**Tempo** : 2 ore e 30 minuti


La struttura dell'output atteso è la seguente:

```
$ ./file-size-2 /usr/bin /usr/include/
```
```
[DIR-1] scansione della cartella '/usr/bin'...
[DIR-2] scansione della cartella '/usr/include/'...
[DIR-2] trovato il file 'aio.h' di 7457 byte; l'insieme ha adesso 1 elementi
[DIR-2] trovato il file 'aliases.h' di 2032 byte; l'insieme ha adesso 2
elementi
[DIR-1] trovato il file '411toppm' di 18504 byte; ; l'insieme ha adesso 3
elementi
[ADD-1] il minimo (2032) e il massimo ( 18504 ) sono stati sostituiti da 20536; ;
l'insieme ha adesso 2 elementi
[DIR-1] trovato il file 'anytopnm' di 5680 byte; ; l'insieme ha adesso 3
elementi
[ADD-2] il minimo (5680) e il massimo (20536) sono stati sostituiti da 26216;
l'insieme ha adesso 2 elementi
[DIR-2] trovato il file 'alloca.h' di 1204 byte; l'insieme ha adesso 3 elementi

...

[MAIN] i thread secondari hanno terminato e il totale finale è di 166389312
byte
```
