# Palindrome Filter
**Tempo a disposizione**: 2 ore

Creare un programma **palindrome-filter.c** in linguaggio C che accetti invocazioni sulla riga di comando del tipo:
```bash
palindrome-filter <input-file>
```

Il programma dovrà fungere da filtro per selezionare, tra le parole in input, quelle che rappresentano una parola palindroma. L'input atteso è una lista di parole (una per riga) dal file specificato sulla riga di comando. L'output risultato della selezione verrà riversato sullo standard output.

Il programma, al suo avvio, creerà 3 thread **R**, **P** e **W** che avranno accesso ad una struttura dati condivisa a cui accederanno con mutua esclusione utilizzando un certo numero di semafori (minimo a scelta dello studente).

I ruoli dei 3 thread saranno i seguenti:
- il thread **R** leggerà il file riga per riga e inserirà, ad ogni iterazione, la riga letta (parola) all'interno della struttura dati condivisa;
- il thread **P** analizzerà, ad ogni iterazione, la parola inserita da **R** nella struttura dati; se la parola è palindroma, P dovrà passarla al thread **W** "svegliandolo";
- il thread **W**, ad ogni "segnalazione" di P, scriverà sullo standard output la parola palindroma.



## Esempio di esecuzione
```bash
palindrome-filter /usr/share/dict/words

è
a
acca
aerea
afa
aia
alla
alula
anilina
anona
ara
arra
atta
azza
b
bob
c
CFC
CPC
d
DVD
e
ebbe
ecce
effe
eme
emme
enne
erre
f
g
gag
h
i
ICI
idi
ivi
j
k
kayak
l
m
n
non
o
odo
oidio
omo
oro
ossesso
osso
ottetto
otto
p
pop
q
r
radar
s
SMS
SOS
t
tot
u
uau
v
w
www
x
xx
xxx
y
z
```