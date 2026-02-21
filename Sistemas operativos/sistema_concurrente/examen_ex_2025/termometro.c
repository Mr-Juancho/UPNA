#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>


int tomaMedida() {
 return rand() % 100;
} 

// ./termometro clave indice periodoTermo umbralTermo
int main(int argc, char *argv[]) {

    if (argc < 5){
        fprintf(stderr, "Numero de argumentos invalido\n");
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int indice = atoi(argv[2]);
    int periodoTermo = atoi(argv[3]);   
    int umbralTermo = atoi(argv[4]);   

    //misma clave que usa meteoestacion
    key_t clave_memoria = ftok(clave, 'A');
    if (clave_memoria == -1) {
        perror("surtidor: ftok");
        exit(EXIT_FAILURE);
    }

    //memoria compartida
    int id_memoria = shmget(clave_memoria, 0, 0666);
    if (id_memoria == -1){
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }
    int *temps = shmat(id_memoria, NULL, 0);
    if (temps == (void *)-1) {
        perror("Error al vincular la memoria compartida");
        exit(EXIT_FAILURE);
    }


    /* grupo de semáforos:
       0 -> tinta deposito
       1 -> mutex capacidad_surtidor */
    int id_semaforo = semget(clave_memoria, 2, 0666);
    if (id_semaforo == -1) {
        perror("surtidor: semget");
        exit(EXIT_FAILURE);
    }

    struct sembuf op;

    while (1) {
        sleep(periodoTermo);

        op.sem_num = 1; //mutex
        op.sem_op = -1; //ocupado
        op.sem_flg = 0;

        if (semop(id_semaforo, &op, 1) == -1) {
            perror("termometro: semop mutex");
            exit(EXIT_FAILURE);    
        }

        temps[indice] = tomaMedida();
        if (temps[indice] > umbralTermo){
            if (kill(getppid(), SIGUSR1) == -1) perror("termometro: kill(SIGUSR1)");
        }

        //libera el semaforo
        op.sem_op = 1;
        if (semop(id_semaforo, &op, 1) == -1) {
            perror("surtidor: semop up mutex");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
