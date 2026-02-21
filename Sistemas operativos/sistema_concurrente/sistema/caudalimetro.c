#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

struct fluido {
    int contador;
    int caudal;
};

struct mensaje {
    long tipo;
    int pid;
    char texto[100];
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Uso: %s clave umbral\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int umbral = atoi(argv[2]);

    key_t clave_ipc = ftok(clave, 'A');
    if (clave_ipc == -1) {
        perror("caudalimetro: ftok");
        exit(EXIT_FAILURE);
    }

    int id_cola = msgget(clave_ipc, 0666);
    if (id_cola == -1) {
        perror("caudalimetro: msgget");
        exit(EXIT_FAILURE);
    }

    struct fluido f;
    struct mensaje msg;

    while (1) {
        // leer struct fluido del surtidor (stdin) 
        size_t r = fread(&f, sizeof(struct fluido), 1, stdin);
        if (r == 0)   /* EOF */
            break;
        if (r != 1) {
            perror("caudalimetro: fread fluido");
            continue;
        }

        sleep(1);

        // mensaje por stderr, no por stdout
        fprintf(stderr,"Caudal instantaneo: %d litros (descarga %d)\n",f.caudal, f.contador);
        fflush(stderr);

        // reenviar struct fluido a sumidero por stdout (pipe limpio) 
        if (fwrite(&f, sizeof(struct fluido), 1, stdout) != 1) {
            perror("caudalimetro: fwrite to pipe");
        }
        fflush(stdout);

        // alerta si caudal > umbral 
        if (f.caudal > umbral) {
            msg.tipo = 2;
            msg.pid  = getpid();
            snprintf(msg.texto, sizeof(msg.texto),"Problema de caudal excesivo en %d", msg.pid);

            if (msgsnd(id_cola,&msg,sizeof(struct mensaje) - sizeof(long),0) == -1) {
                perror("caudalimetro: msgsnd alerta");
            }
        }
    }

    return 0;
}
