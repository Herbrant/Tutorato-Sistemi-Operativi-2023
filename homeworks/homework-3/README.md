# Homework 3
Usando la possibilità di mappare file in memoria, creare un programma che
possa manipolare un file arbitrariamente grande costituito da una sequenza
di record lunghi N byte.
La manipolazione consiste nel riordinare, tramite un algoritmo di ordinamento
a scelta, i record considerando il contenuto dello stesso come chiave:
ovvero, supponendo N=5, il record [4a a4 91 f0 01] precede [4a ff 10 01 a3].
La sintassi da supportare è la seguente:
```bash
$ homework-3 <N> <pathname del file>
```

È possibile testare il programma sul file 'esempio.txt' prodotto dal seguente
comando, utilizzando il parametro N=33:

```bash    
$ ( for I in `seq 1000`; do echo $I | md5sum | cut -d' ' -f1 ; done ) > esempio.txt
```
Su tale file, l'output atteso e' il seguente:

```bash
$ homework-3 33 esempio.txt

$ head -n5 esempio.txt
    000b64c5d808b7ae98718d6a191325b7
    0116a06b764c420b8464f2068f2441c8
    015b269d0f41db606bd2c724fb66545a
    01b2f7c1a89cfe5fe8c89fa0771f0fde
    01cdb6561bfb2fa34e4f870c90589125

$ tail -n5 esempio.txt
    ff7345a22bc3605271ba122677d31cae
    ff7f2c85af133d62c53b36a83edf0fd5
    ffbee273c7bb76bb2d279aa9f36a43c5
    ffbfc1313c9c855a32f98d7c4374aabd
    ffd7e3b3836978b43da5378055843c67
```