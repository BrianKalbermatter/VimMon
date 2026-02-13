/*
 * ============================================================
 * FASE 4 - Ejercicio 4.1: micat - Clon de cat en C
 * ============================================================
 * OBJETIVO: Practicar manejo de archivos, argumentos y I/O en C
 *
 * REQUISITOS:
 * 1. Recibir un archivo como argumento
 * 2. Mostrar su contenido (como cat)
 * 3. Flag -n: mostrar numeros de linea
 * 4. Flag -c: contar lineas, palabras y caracteres
 * 5. Manejar errores con perror()
 * 6. Leer por bloques (no cargar todo en memoria)
 *
 * COMPILAR: gcc -Wall -Wextra -o micat ejercicio_4.1_micat.c
 * USO:      ./micat archivo.txt
 *           ./micat -n archivo.txt
 *           ./micat -c archivo.txt
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 4096

void usage(const char *prog) {
    fprintf(stderr, "Uso: %s [-n] [-c] <archivo>\n", prog);
    fprintf(stderr, "  -n  Mostrar numeros de linea\n");
    fprintf(stderr, "  -c  Contar lineas, palabras y caracteres\n");
    exit(1);
}

/* Mostrar contenido del archivo */
void cat_file(const char *filename, int show_numbers) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        exit(1);
    }

    char line[BUFFER_SIZE];
    int line_num = 1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        if (show_numbers) {
            printf("%6d\t%s", line_num++, line);
        } else {
            printf("%s", line);
        }
    }

    fclose(fp);
}

/* Contar lineas, palabras y caracteres */
void count_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror(filename);
        exit(1);
    }

    int lines = 0, words = 0, chars = 0;
    int in_word = 0;
    int c;

    while ((c = fgetc(fp)) != EOF) {
        chars++;

        if (c == '\n')
            lines++;

        if (c == ' ' || c == '\n' || c == '\t') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            words++;
        }
    }

    fclose(fp);

    printf("  %d lineas\n", lines);
    printf("  %d palabras\n", words);
    printf("  %d caracteres\n", chars);
    printf("  %s\n", filename);
}

int main(int argc, char *argv[]) {
    int show_numbers = 0;
    int count_mode = 0;
    const char *filename = NULL;

    /* Parsear argumentos */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            show_numbers = 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            count_mode = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Opcion desconocida: %s\n", argv[i]);
            usage(argv[0]);
        } else {
            filename = argv[i];
        }
    }

    if (!filename) {
        usage(argv[0]);
    }

    if (count_mode) {
        count_file(filename);
    } else {
        cat_file(filename, show_numbers);
    }

    return 0;
}
