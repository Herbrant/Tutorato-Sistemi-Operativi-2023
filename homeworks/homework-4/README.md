# Homework 4
Estendere l'esercizio 'homework n.1' affinche' operi correttamente
anche nel caso in cui tra le sorgenti e' indicata una directory, copiandone
il contenuto ricorsivamente. Eventuali link simbolici incontrati dovranno
essere replicati come tali (dovrà essere creato un link e si dovranno
preservare tutti permessi di accesso originali dei file e directory).

Una ipotetica invocazione potrebbe essere la seguente:
```bash
$ homework-4 directory-di-esempio file-semplice.txt path/altra-dir/ "nome con spazi.pdf" directory-destinazione
```
