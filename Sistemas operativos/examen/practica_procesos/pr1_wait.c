#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){

    int pid;
    int status;

    if((pid = fork()) == -1){
        printf("error al crear el proceso");
        return 1;
    }else if(pid == 0){
        printf("Soy el hijo con el pid %ld y mi padre tiene el pid %ld", (long)getpid(), (long)getppid());
        fflush(stdout);
        return 2;
    }else {
        wait(&status);
        printf("\nYo soy el padre, mi pid es %ld y espero a que mi hijo termine",(long)getpid());
        return 3;
    }


    return 0;
}