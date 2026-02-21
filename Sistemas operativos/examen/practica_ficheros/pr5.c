#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]){

    if (argc < 2){
        fprintf(stderr,"Uso: %s <Nombre_archivo>", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "r+");
    char cadena[100] = "Hola Mundo version 2.0\n";
    sleep(5);
    fputs(cadena, fp);

    fclose(fp);

    return 0;
}