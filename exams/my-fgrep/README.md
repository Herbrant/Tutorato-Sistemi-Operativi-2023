# my-fgrep

Creare un programma **my-fgrep.c** in linguaggio C che accetti invocazioni sulla riga di
comando del tipo:

```bash
my-fgrep [-v] [-i] <word> <file-1> [file-2] [file-3] [...]
```
Il programma emula il comportamento dell'omonimo comando di shell: legge
*sequenzialmente* il contenuto dei file indicati e seleziona le righe contenenti la parola specificata; con l'opzione **v** il controllo viene invertito (vengono selezionate le righe che NON contengono la parola); con l'opzione **i** il controllo diventa *case-insensitive*; ogni riga selezionata viene riportata nello standard-output usando il nome del file di origine come prefisso.

All'avvio, il programma creerà i seguenti thread:

- un thread **Reader** per ogni file indicato sulla riga di comando: ognuno di essi leggerà il file assegnato utilizzando la mappatura dei file in memoria e ne invierà il contenuto, riga per riga, al thread **Filterer**; questi thread dovranno operare in modo seriale (ad esempio: il secondo sarà attivato solo quando il primo avrà finito, e così via...);
- un unico thread **Filterer**: per ogni riga ricevuta applicherà il criterio di selezione specificato e manderà le righe risultati, complete di prefisso, indietro al thread  **Writer** che le riporterà in output.
- un unico thread **Writer**: si occuperà di stampare in output ogni riga ricevuta dal thread **Filterer** 

I thread **Reader** condivideranno una struttura dati con il thread **Filterer** che conterrà una riga di testo e l'indicazione del file di origine (stabilire delle dimensioni massime per la riga e per il path del file). Il thread **Filterer**, inoltre, condividerà una seconda struttura dati con il main **Writer**.

Tutti i thread dovranno coordinarsi utilizzando un mutex e un certo numero di variabili condizione.

La fase di terminazione dei thread (al termine dei lavori) dovrà essere implementata facendo uso di una barriera piuttosto che utilizzando la classica funzione *pthread_join* (il cui uso non è permesso).

Al termine dei lavori, tutte le strutture dati impiegate dovranno essere deallocate.


Un esempio di output potrebbe essere il seguente:

```
$ cat nomi.txt
Marco
Alberto
Danilo
Matteo
Alfio
Vincenzo
Andrea
```
```
$ cat cognomi.txt
Mezzasalma
Facchi
Donetti
Cannella
Faro
Filetti
Cirrincione
```
```
$ ./my-fgrep -i al nomi.txt cognomi.txt
nomi.txt:Alberto
nomi.txt:Alfio
cognomi.txt:Mezzasalma
```
```
$ ./my-fgrep -v i cognomi.txt nomi.txt
cognomi.txt:Mezzasalma
cognomi.txt:Cannella
cognomi.txt:Faro
nomi.txt:Marco
nomi.txt:Alberto
nomi.txt:Matteo
nomi.txt:Andrea
```
Note/suggerimenti:

- per l'analisi del testo possono essere utili le funzioni di libreria **strstr()** e
    **strcasestr()**.

**Tempo** : 2 ore e 30 minuti
