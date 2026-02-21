#include <stdio.h>
#include <string.h>

int main(){

    FILE *archivo = fopen("prueba.txt", "r");
    char cadena[100];

    if(archivo == NULL)printf("Error, al leer el archivo");

    while(fgets(cadena, 100, archivo) != NULL){
        printf("%s", cadena);
    }


    return 0;
}