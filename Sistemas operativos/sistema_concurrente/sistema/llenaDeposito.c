#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(stderr, "Uso: %s clave tiempo volumen\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int tiempo  = atoi(argv[2]);
    int volumen = atoi(argv[3]);

    key_t clave_ipc = ftok(clave, 'A');
    if (clave_ipc == -1) {
        perror("llenaDeposito: ftok");
        exit(EXIT_FAILURE);
    }

    int id_semaforo = semget(clave_ipc, 2, 0666);
    if (id_semaforo == -1) {
        perror("llenaDeposito: semget");
        exit(EXIT_FAILURE);
    }

    sleep(tiempo);

    struct sembuf op;
    op.sem_num = 0;       
    op.sem_op  = volumen; 
    op.sem_flg = 0;

    if (semop(id_semaforo, &op, 1) == -1) {
        perror("llenaDeposito: semop");
        exit(EXIT_FAILURE);
    }
    printf("llenaDeposito: añadido %d litros al depósito.\n", volumen);
    return 0;
}
