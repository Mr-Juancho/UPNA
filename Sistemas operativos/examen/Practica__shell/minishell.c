#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <stdbool.h>
#include "fragmenta.h"

int main(void){
	char* cadena;
	cadena = (char*)malloc(100 * sizeof(char));
	printf("miniShell\\>");
	fflush(stdout);
	scanf("%[^\n]",cadena);
	getchar();
	while(strcmp(cadena,"exit") != 0){
		
		if(strlen(cadena) > 0){
			int pid;
			pid = fork();
		
			if(pid == 0 ){
				char** arg;
				arg = fragmenta(cadena); //divide el texto
				//execvp - NULL como ultima

				int i = 0;
				while(arg[i] != NULL){
					if( strcmp(arg[i],">") == 0){
						int fd;
						fd = open(arg[i+1], O_CREAT | O_WRONLY | O_TRUNC , 0600);
						dup2(fd, STDOUT_FILENO);
						close(fd);
						free(arg[i]);
						free(arg[i+1]);
						arg[i] = NULL;
						arg[i+1] = NULL;

					}else if(strcmp(arg[i],">>") == 0){
						int fd;
						fd = open(arg[i+1], O_CREAT | O_RDWR | O_APPEND , 0600); // Tercer argumento solo tiene sentido cuando lo creo.
						dup2(fd, STDOUT_FILENO);
						close(fd);
						free(arg[i]);
						free(arg[i+1]);
						arg[i] = NULL;
						arg[i+1] = NULL;

					}else if(strcmp(arg[i],"<") == 0){
						int fd;
						fd = open(arg[i+1], O_RDONLY); // No tiene sentido el tercer argumento.
						dup2(fd, STDOUT_FILENO);
						close(fd);
						free(arg[i]);
						free(arg[i+1]);
						arg[i] = NULL;
						arg[i+1] = NULL;

					}else if(strcmp(arg[i],"|") == 0){
						int tuberia[2];
						pipe(tuberia);
						int pid;
						free(arg[i]);	//.  | encontrado(tuberia encontrada)
						arg[i] = NULL; //.    asignamos el lugar de la tuberia a NULL
						pid = fork();
						if (pid == -1){
							perror("No se pudo asignar memoria");
							exit(1);
						}else if(pid == 0){
							dup2(tuberia[1], STDOUT_FILENO);
							close(tuberia[0]);
							execvp(arg[0],arg);
							printf("ERROR\n");
							exit(1);
						}else{
							dup2(tuberia[0], STDIN_FILENO);
							close(tuberia[1]);

							char **arg2;
							arg2 = &arg[i+1];
							execvp(arg2[0],arg2);
							printf("ERROR\n");
							exit(1);
						}
					}
					i += 1;
				}


				execvp(arg[0],arg);
				printf("error\n");
				exit(1);
			}else{
				wait(NULL);
				
			}
		
		}
		printf("miniShell\\>");
		fflush(stdout);
		cadena[0] = '\0';
		scanf("%[^\n]",cadena);
		getchar();
    }
    return 0;
}
    