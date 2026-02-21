#include <stdio.h>
#include <string.h>
#include <ctype.h>


int main(int argc, char *argv[]){


    if (argc < 2){
        fprintf(stderr, "formato error, %s <nombre-archivo>",argv[0]);
        return 1;
    }

    FILE *archivo = fopen(argv[1], "r+");
    char cadena1[100];
    char entrada[100];
    char cadena2[100];

    if (archivo == NULL){
        fprintf(stderr,"Error al abrir el archivo");
        fclose(archivo);
        return 2;
    }

    while((fgets(cadena1,100, archivo)) != NULL){
        printf("%s ", cadena1);
    }

    if ((fgets(entrada,100,stdin)) != NULL){
        fputs(entrada, archivo);
        printf("Introducido %s",entrada);
    }

    while((fgets(cadena2,100, archivo)) != NULL){
        printf("%s ", cadena2);
    }

    fclose(archivo);

    return 0;

}