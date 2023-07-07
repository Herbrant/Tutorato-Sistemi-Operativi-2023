# File size
Creare un programma **file-size.c** in linguaggio C che accetti invocazioni sulla riga di comando del tipo:
```bash
file-size <dir-1> <dir-2> ... <dir-*n*>
```

Il programma dovrà determinare la dimensione totale in byte dei file regolari direttamente contenuti all'interno delle cartelle indicate (senza ricorsione).

Al suo avvio il programma creerà *n+1* thread:
- *n* thread DIR-*i* che si occuperanno di scansionare la cartella assegnata alla ricerca di file regolari direttamente contenuti in essa (no ricorsione);
- un thread STAT che si occuperà di determinare la dimensione di ogni file regolare individuato.

Gli *n* thread DIR-*i* agiranno in parallelo e inseriranno, per ogni file regolare incontrato, il pathname dello stesso all'interno di un buffer condiviso di capienza prefissata (10 pathname). Il thread STAT estrarrà, uno alla volta, i pathname da tale buffer e determinerà la dimensione in byte del file associato. La coppia di informazioni (pathname, dimensione) sarà passata, attraverso un'altra struttura dati, al thread principale MAIN che si occuperà di mantenere un totale globale.

I thread si dovranno coordinare opportunamente tramite mutex e semafori numerici POSIX: il numero (minimo) e le modalità di impiego sono da determinare da parte dello studente. Si dovrà inoltre rispettare la struttura dell'output riportato nell'esempio a seguire.

I thread dovranno terminare spontaneamente al termine dei lavori.

**Tempo**: 2 ore e 30 minuti

## Esempio di output
La struttura dell'output atteso è la seguente:
```
$ ./file-size /usr/bin /usr/include/
[D-1] scansione della cartella '/usr/bin'...
[D-2] scansione della cartella '/usr/include/'...
[D-2] trovato il file 'aio.h' in '/usr/include/'
[D-2] trovato il file 'aliases.h' in '/usr/include/'
[STAT] il file '/usr/include/aio.h' ha dimensione 7457 byte
[D-1] trovato il file '411toppm' in '/usr/bin'
[STAT] il file '/usr/include/aliases.h' ha dimensione 2032 byte
[MAIN] con il file '/usr/include/aio.h' il totale parziale è di 7457 byte
[MAIN] con il file '/usr/include/aliases.h' il totale parziale è di 9489 byte
[D-1] trovato il file 'add-apt-repository' in '/usr/bin'
[STAT] il file '/usr/bin/411toppm' ha dimensione 18504 byte
...
[MAIN] il totale finale è di 166389312 byte
```