#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>


volatile sig_atomic_t terminar = 0;

void manejador_señal(int signum){
    terminar += 1;
}

int main(){

    int pid;

    if ((pid=fork()) == -1){
        printf("Error");
        return 1;
    }else if (pid == 0){
        //recibe la señal
        signal(SIGUSR1, manejador_señal);
        
        for (int i = 0; i <2; i++){
            pause();
            //seguimos
            printf("El valor desde el hijo es %d", terminar);
            kill(getppid(), SIGUSR2);
        }
        
    }else{
        //manda la señal
        signal(SIGUSR2, manejador_señal);

        for(int j = 0; j<2; j++){
            kill(pid, SIGUSR1);

            pause();
            printf("El valor desde el padre es %d", terminar);
        }

        wait(NULL);
        printf("Padre termina.\n");
    }



    return 0;
}