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
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"Error");
            return 1;
        }else if(pid == 0){
            fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
            pid = fork();
            if(pid == -1){
                fprintf(stderr,"Error");
                return 1;
            }else if(pid == 0){
                fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
                sleep(3);
            }else{
                wait(NULL);
            }
        }else{
            pid = fork();
            if(pid == -1){
                fprintf(stderr,"Error");
                return 1;
            }else if(pid == 0){
                fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
                sleep(3);
            }else{
                wait(NULL);
            }
        }
    }else{
        fprintf(stdout,"Soy el padre es (%ld)\n",(long)getpid());
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"Error");
            return 1;
        }else if(pid == 0){
            fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
            pid = fork();
            if(pid == -1){
                fprintf(stderr,"Error");
                return 1;
            }else if(pid == 0){
                fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
                sleep(3);
            }else{
                wait(NULL);
            }
        }else{
            fprintf(stdout,"Soy el padre es (%ld)\n",(long)getpid());
            pid = fork();
            if(pid == -1){
                fprintf(stderr,"Error");
                return 1;
            }else if(pid == 0){
                fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
                pid = fork();
                if(pid == -1){
                    fprintf(stderr,"Error");
                    return 1;
                }else if(pid == 0){
                    fprintf(stdout,"Soy el hijo (%ld), mi padre es (%ld)\n",(long)getpid(), (long)getppid());
                    sleep(3);
                }else{
                    wait(NULL);
                }
            }else{
                fprintf(stdout,"Soy el padre es (%ld)\n",(long)getpid());
                pid = fork();
                wait(NULL);
                
            }
            wait(NULL);
            
        }
        wait(NULL);
    }


    return 0;
}