#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *swap_buffer; // riferimento ad buffer di scambio globale

/* trova il pivot in map[i]...map[j] restituendone una copia (allocando la
   memoria); restituisce NULL se gli elementi sono tutti uguali */
char *search_pivot(char *map, int i, int j, int size) {
    int k;

    for (k = i + 1; k <= j; k++) {
        if (memcmp(map + size * k, map + size * i, size) > 0) // map[k] > map[i]
            return (
                memcpy(malloc(size), map + size * k, size)); // pivot = map[k]
        else if (memcmp(map + size * k, map + size * i, size) <
                 0) // map[k] < map[i]
            return (
                memcpy(malloc(size), map + size * i, size)); // pivot = map[i]
    }
    return (NULL);
}

// partiziona map[p]...map[r] usando il pivot
int partition(char *map, int p, int r, char *pivot, int size) {
    int i, j;

    i = p;
    j = r;
    do {
        while (memcmp(map + size * j, pivot, size) >= 0) // map[j] >= pivot
            j--;
        while (memcmp(map + size * i, pivot, size) < 0) // map[i] < pivot
            i++;
        if (i < j) { // map[i] <-> map[j]
            memcpy(swap_buffer, map + size * i, size);
            memcpy(map + size * i, map + size * j, size);
            memcpy(map + size * j, swap_buffer, size);
        }
    } while (i < j);
    return (j);
}

// quicksort in versione ricorsiva
void quicksort(char *map, int p, int r, int size) {
    int q;
    char *pivot;

    pivot = search_pivot(map, p, r, size);
    if ((p < r) && (pivot != NULL)) {
        q = partition(map, p, r, pivot, size);
        quicksort(map, p, q, size);
        quicksort(map, q + 1, r, size);
    }
    if (pivot)
        free(pivot);
}

int main(int argc, char *argv[]) {
    struct stat sb;
    int size, i, num_records;
    char *map;
    long fd;

    if (argc < 3) {
        fprintf(stderr, "uso: %s <dimensione record> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((fd = open(argv[2], O_RDWR)) == -1) {
        perror(argv[2]);
        exit(EXIT_FAILURE);
    }

    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }
    if (!S_ISREG(sb.st_mode)) {
        fprintf(stderr, "%s non è un file\n", argv[2]);
        exit(EXIT_FAILURE);
    }

    size = atoi(argv[1]);
    if ((size <= 0) || ((sb.st_size % size) != 0)) {
        fprintf(stderr,
                "dimensione del record %d non valida o dimensione del file non "
                "congruente!\n",
                size);
        exit(EXIT_FAILURE);
    }

    if ((map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                    0)) == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    // alloca il buffer di scambio globale (al posto di uno locale alla
    // procedura 'partition'
    swap_buffer = malloc(size);

    // ordina il contenuto del file
    num_records = sb.st_size / size;
    quicksort(map, 0, num_records - 1, size);

    // libera la memoria del buffer una volta finito
    free(swap_buffer);

    printf("Record del file '%s' riordinati!\n", argv[2]);

    if (munmap(map, sb.st_size) == -1) {
        perror("munmap");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
