#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define MODE 0600
#define BUFFER_SIZE 2048

char *create_dest_path(char *source, char *dest) {
    char *path =
        calloc(PATH_MAX, sizeof(char)); // malloc(sizeof(char) * PATH_MAX)
    sprintf(path, "%s/%s", dest, basename(source));

    return path;
}

void copy(char *source, char *dest) {
    int sd, dd, size;
    char buffer[BUFFER_SIZE];

    if ((sd = open(source, O_RDONLY)) == -1) {
        perror(source);
        exit(EXIT_FAILURE);
    }

    char *path = create_dest_path(source, dest);

    if ((dd = open(path, O_WRONLY | O_CREAT | O_TRUNC, MODE)) == -1) {
        perror(path);
        exit(EXIT_FAILURE);
    }

    do {
        if ((size = read(sd, buffer, BUFFER_SIZE)) == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        if (write(dd, buffer, size) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
    } while (size == BUFFER_SIZE);

    free(path);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <file-1> [file-2] ... [file-n] <dest-dir>\n",
               argv[0]);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc - 1; i++)
        copy(argv[i], argv[argc - 1]);

    exit(EXIT_SUCCESS);
}