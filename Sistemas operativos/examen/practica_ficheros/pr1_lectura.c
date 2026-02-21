#include <stdio.h>


int main(int argc, char *argv[]){

    FILE *fp = fopen("prueba.txt", "r");

    if (fp == NULL){
        printf("Error de apertura del archivo");
    }else{
        char caracter;
        while((caracter=fgetc(fp)) != EOF){
            if(caracter != '\n' && caracter != '\r'){
                printf("%c - ", caracter);
            }
                
        }
    }
    return 0;
}