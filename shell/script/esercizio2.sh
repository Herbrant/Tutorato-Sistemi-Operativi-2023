#!/bin/bash

# Verifichiamo se l'utente ha invocato lo script passando i parametri necessari
[ $# -lt 2 ] && echo "Usage: $0 pathname string" && exit 1

# Verifichiamo se il file NON è regolare
[ ! -f "$1" ] && echo "Errore: il file $1 non è regolare" && exit 2

# Utilizziamo grep per cercare le righe contenenti la stringa all'interno del file e
# contiamo il numero di righe utilizzando wc
grep "$2" "$1" | wc -l 

exit 0