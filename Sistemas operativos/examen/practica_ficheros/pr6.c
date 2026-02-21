#include <stdio.h>


int main(int argc, char *argv[]){

    if (argc <2){
        fprintf(stderr,"Uso: %s <Nombre_archivo>", argv[0]);
        return 1;
    }

    FILE *archivo = fopen(argv[1], "r+");
    if(archivo == NULL){
        fprintf(stderr,"Error, nombre de archivo no se encuentra");
        return 2;
    }

    char cadena[300];
    while((fgets(cadena,300,archivo)) != NULL){
        fprintf(stdout,"%s", cadena);
    }

    fclose(archivo);

    return 0;

}