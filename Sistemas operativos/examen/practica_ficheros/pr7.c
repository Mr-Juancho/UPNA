#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <archivo_entrada> <archivo_salida>\n", argv[0]);
        return 1;
    }

    FILE *entrada = fopen(argv[1], "r");
    if (entrada == NULL) {
        perror("Error abriendo archivo de entrada");
        return 1;
    }

    FILE *salida = fopen(argv[2], "w");
    if (salida == NULL) {
        perror("Error abriendo archivo de salida");
        fclose(entrada);
        return 1;
    }

    char caracter;
    while ((caracter = fgetc(entrada)) != EOF) {
        if (strchr("aeiou", caracter))
            fputc(toupper(caracter), salida);
        else
            fputc(caracter, salida);
    }

    fclose(entrada);
    fclose(salida);
    return 0;
}
