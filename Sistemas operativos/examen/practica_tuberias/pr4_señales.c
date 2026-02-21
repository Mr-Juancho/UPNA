//senales
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>



volatile sig_atomic_t terminar = 0; //global

void manejador_señal(int signum){
    terminar = 1;
}


int main(){

    int pid;


    if ((pid = fork()) == -1){
        fprintf(stderr,"Error");
        return 1;
    }else if (pid == 0){
        //mandar la señal
        sleep(4);
        kill(getppid(), SIGINT);

    }else{
        //recibir la señal
        signal(SIGINT, manejador_señal);
        while(!terminar){
            printf("hola\n");
            sleep(1);
        }
        wait(NULL);
    }



    return 0;
}