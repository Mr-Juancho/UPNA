#include <stdio.h>
#include <string.h>

int main(){

    FILE *archivo = fopen("prueba.txt", "r+");
    char caracter;

    if(archivo == NULL)printf("Error, al leer el archivo");

    printf("Introduce un caracter al fichero\n");

    while ((caracter = getchar()) != '\n')
    {
        fputc(caracter, archivo);
    }
    return 0;
}