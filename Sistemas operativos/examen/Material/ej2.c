#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>

int main(){

    int pid,pid_aux;
    
    pid = fork();
    if(pid == -1){
        fprintf(stderr,"Error");
        return 1;
    }else if(pid == 0){
        fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
        sleep(3);
        
        pid_aux=fork();
        if(pid_aux == 0){
            fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
            sleep(3);
        
            pid_aux=fork();
            if(pid_aux == 0){
                fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
                sleep(3);
                exit(2);
            }else{
                wait(NULL);
            }
            exit(2);
        }else{
            wait(NULL);
        }
        exit(2);
    }else{
        wait(NULL);
    }


    return 0;
}