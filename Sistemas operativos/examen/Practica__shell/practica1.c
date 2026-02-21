#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // fork, execvp
#include <sys/types.h> // pid_t
#include <sys/wait.h>  // wait
#include "fragmenta.h"

int main(void) {

    char *cadena;
    cadena = (char *)malloc(100 * sizeof(char));
    
    printf("minishell\\>");
    fflush(stdout);

    scanf("%99[^\n]", cadena);
    getchar();

    while(strcmp(cadena,"exit") !=0){
        int pid = fork();
        if(pid == 0){
            char **arg;
            arg = fragmenta(cadena);
            execvp(arg[0], arg);
            printf("Error, comando no existe o no se encuentra\n");
            exit(1);
        }else{
            wait(NULL);
            printf("minishell\\>");
            fflush(stdout);
            scanf("%[^\n]", cadena); //se queda el intro al final
            getchar();
        }
    }

    return 0;
}


