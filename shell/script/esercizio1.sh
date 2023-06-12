#!/bin/bash
readonly OUTPUT_FILE=merged.txt

# Verifichiamo se l'utente ha invocato lo script passando i parametri necessari
[ $# -eq 0 ] && echo "Usage: $0 [file-1] [file-2] ... [file-n]" && exit 1

echo '' > "$OUTPUT_FILE" # Creaiamo o azzeriamo il contenuto del file di output

for f in "$@"; do # Iteriamo sui parametri
    if [ -f "$f" ]; then    # Verifichiamo se f è un file regolare
        cat $f >> "$OUTPUT_FILE"
    else
        echo "Errore: il file $f non è regolare"
        exit 1
    fi
done

exit 0