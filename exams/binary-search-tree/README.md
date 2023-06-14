# Binary Search Tree
Creare un programma **binary-search-tree.c** in linguaggio C che accetti invocazioni sulla riga di comando del tipo:
```bash
binary-search-tree <input-file-1> <input-file-2> <...>
```

Al suo avvio, il programma creerà un numero, non predeterminati, di thread pari a quello dei file di input indicati sulla riga di comando. Tali thread comunicheranno con il thread padre attraverso un certo numero di semafori (minimo a scelta dello studente).

Ciascun thread, al suo avvio creerà e terrà in memoria un *binary-search-tree* di interi a partire dal file a lui destinato. Il file conterrà un certo numero di chiavi, discuna delle quali separata da un ritorno a capo (`\n`).

Il programma dovrà offrire una shell interattiva che accetta le seguenti richieste:
- **add n key**: aggiunge un nodo con chiave *key* all'*n*-esimo BST;
- **search n key**: verifica se è presente *key* all'interno del BST *n*;
- **print n**: stampa l'*n*-esimo BST;
- **quit**: chiude tutti i thread, ripulendo opportunamente tutte le strutture dati utilizze.

Il programma dovrà rispettare i seguenti vincoli:
- il thread padre, ovvero quello originario creato dall'invocazione del programma:
    - sarà l'unico a poter interagire con il terminale e a poter visualizzare messaggi;
    - non dovrà mai accedere direttamente al filesystem o ai BST
    - condividerà con gli altri thread una struttura dati (a scelta dello studente)
- il generico thread **T-n**
    - non potrà visualizzare alcun messaggio sul terminale;
    - potrà accedere solo all'*n*-esimo file a lui destinato;
    - comunicherà con il thread principale utilizzando la struttura dati condivisa

Un esempio di file di input potrebbe essere il seguente:
```
10
4
9
1
-10
20
...
2
```
