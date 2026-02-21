#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

struct fluido {
    int contador;
    int caudal;
};

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Uso: %s clave pid_monitor\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    pid_t pid_monitor = (pid_t)atoi(argv[2]);

    key_t clave_ipc = ftok(clave, 'A');
    if (clave_ipc == -1) {
        perror("sumidero: ftok");
        exit(EXIT_FAILURE);
    }

    int id_memoria = shmget(clave_ipc, sizeof(int), 0666);
    if (id_memoria == -1) {
        perror("sumidero: shmget");
        exit(EXIT_FAILURE);
    }

    int *capacidad_surtidor = (int *)shmat(id_memoria, NULL, 0);
    if (capacidad_surtidor == (void *)-1) {
        perror("sumidero: shmat");
        exit(EXIT_FAILURE);
    }

    int id_semaforo = semget(clave_ipc, 2, 0666);
    if (id_semaforo == -1) {
        perror("sumidero: semget");
        exit(EXIT_FAILURE);
    }

    struct sembuf op;
    struct fluido f;

    while (1) {
        size_t r = fread(&f, sizeof(struct fluido), 1, stdin);
        if (r == 0) { 
            break;
        }
        if (r != 1) {
            perror("sumidero: fread fluido");
            continue;
        }

        sleep(2);

        //semaforo 1
        op.sem_num = 1;
        op.sem_op  = -1;
        op.sem_flg = 0;
        if (semop(id_semaforo, &op, 1) == -1) {
            perror("sumidero: down mutex");
            exit(EXIT_FAILURE);
        }

        //seccion critica
        *capacidad_surtidor -= f.caudal;

        //liberamos el semaforo
        op.sem_op = 1;
        if (semop(id_semaforo, &op, 1) == -1) {
            perror("sumidero: up mutex");
            exit(EXIT_FAILURE);
        }

        if (kill(pid_monitor, SIGUSR1) == -1) {
            perror("sumidero: kill SIGUSR1");
        }
    }

    shmdt(capacidad_surtidor);
    return 0;
}
