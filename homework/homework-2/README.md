# Homework 2
Implementare il comando *mv* supportando i seguenti casi:
- spostamento sullo stesso filesystem: individuato tale caso, il file deve essere spostato utilizzando gli *hard link* 
- spostamento cross-filesystem: individuato tale caso, il file deve essere
    spostato utilizzando la strategia "copia & cancella";
- spostamento di un link simbolico: individuato tale caso, il link simbolico
    deve essere ricreato a destinazione con lo stesso contenuto (ovvero il percorso
    che denota l'oggetto referenziato); notate come tale spostamento potrebbe
    rendere il nuovo link simbolico non effettivamente valido.

La sintassi da supportare è la seguente:

```bash
$ homework-2 <pathname sorgente> <pathname destinazione>
```