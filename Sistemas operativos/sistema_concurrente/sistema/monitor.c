#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

struct mensaje {
    long tipo;
    int  pid;
    char texto[100];
};

void handler(int sig) {
    (void)sig; 
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s clave\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);

    key_t clave_ipc = ftok(clave, 'A');
    if (clave_ipc == -1) {
        perror("monitor: ftok");
        exit(EXIT_FAILURE);
    }

    int id_cola = msgget(clave_ipc, 0666);
    if (id_cola == -1) {
        perror("monitor: msgget");
        exit(EXIT_FAILURE);
    }

    int id_memoria = shmget(clave_ipc, sizeof(int), 0666);
    if (id_memoria == -1) {
        perror("monitor: shmget");
        exit(EXIT_FAILURE);
    }

    int *capacidad_surtidor = (int *)shmat(id_memoria, NULL, 0);
    if (capacidad_surtidor == (void *)-1) {
        perror("monitor: shmat");
        exit(EXIT_FAILURE);
    }

    int id_semaforo = semget(clave_ipc, 2, 0666);
    if (id_semaforo == -1) {
        perror("monitor: semget");
        exit(EXIT_FAILURE);
    }

    struct sembuf op;

    pid_t pid_hijo = fork();
    if (pid_hijo == -1) {
        perror("monitor: fork");
        exit(EXIT_FAILURE);
    }

    if (pid_hijo == 0) {
        //lee mensajes de la cola
        struct mensaje msg;

        while (1) {
            if (msgrcv(id_cola,&msg,sizeof(struct mensaje) - sizeof(long),-2,0) == -1) {
                perror("monitor (hijo): msgrcv");
                continue;
            }

            printf("ALERTA (tipo %ld) de PID %d: %s\n",msg.tipo, msg.pid, msg.texto);
            fflush(stdout);
        }

        shmdt(capacidad_surtidor);
        exit(0);
    } else {
        // PADRE: espera SIGUSR1 del sumidero y muestra nivel del surtidor 
        signal(SIGUSR1, handler);

        while (1) {
            pause();  // se despierta con SIGUSR1 

            // semáforo 1 
            op.sem_num = 1;
            op.sem_op  = -1;
            op.sem_flg = 0;
            if (semop(id_semaforo, &op, 1) == -1) {
                perror("monitor (padre): down mutex");
                exit(EXIT_FAILURE);
            }

            int litros = *capacidad_surtidor;
            printf("Presentes %d litros en el surtidor\n", litros);
            fflush(stdout);
            
            op.sem_op = 1;
            if (semop(id_semaforo, &op, 1) == -1) {
                perror("monitor (padre): up mutex");
                exit(EXIT_FAILURE);
            }
        }

        shmdt(capacidad_surtidor);
    }

    return 0;
}
