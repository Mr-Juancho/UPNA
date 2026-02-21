#include "fragmenta.h"

char **fragmenta(const char *cadena){
    char **arg;
    char *copia;
    //tenemos el problema que no sabemos cuanto ocupa cadena, entonces usamos la libreria string.h
    copia = (char *)malloc((strlen(cadena)+1)*sizeof(char));
    strncpy(copia,cadena,strlen(cadena) + 1);

    int contador = 1;
    char *pal;
    pal = strtok(copia, " ");
    
    while(pal != NULL){
        if(strlen(pal) > 0 ){
            contador ++;
        }
        pal = strtok(NULL, " ");
    }

    arg = (char **)malloc(contador*sizeof(char*));
    int aux= 0;
    strcpy(copia, cadena);
    pal = strtok(copia, " ");

    while(pal != NULL){
        if(strlen(pal) > 0 ){
            arg[aux] = (char *)malloc(strlen(pal)+1);
            strncpy(arg[aux],pal, strlen(pal) + 1);
            aux ++;
        }
        pal = strtok(NULL, " "); 
    }
    arg[aux] = NULL;

    return arg;
}

void borrarg(char **arg){
    int contador = 0;
    while(arg[contador] != NULL){
        free(arg[contador]);
        contador++;
    }
    free(arg);

}