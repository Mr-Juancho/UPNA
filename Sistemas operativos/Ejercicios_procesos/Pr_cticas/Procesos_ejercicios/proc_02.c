#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
int main() {
        static char *cmd[]={"who","ls","date"};
        int i, status;
 
        long unsigned x;
        long int n;
 
        while (1) {
            printf("0=who, 1=ls, 2=date\n");
            scanf("%d",&i);
            if (i<0 || i>2) exit(1);
            if (fork() == 0) {
                execlp(cmd[i], cmd[i], 0);
                printf("Comando no se encontro\n");
                exit(1);
            }
            else wait(&status);
        }

    return 0;
}