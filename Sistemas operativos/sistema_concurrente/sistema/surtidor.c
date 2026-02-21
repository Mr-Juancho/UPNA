#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>      

struct fluido {
    int contador, caudal;
};

struct mensaje {
    long tipo;
    int pid;
    char texto[100];
};

// ./surtidor clave periodo volumen 
int main(int argc, char *argv[]) {

    if (argc < 4){
        fprintf(stderr, "Numero de argumentos invalido\n");
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int periodo = atoi(argv[2]);   
    int volumen = atoi(argv[3]);   

    //misma clave que usa gestor
    key_t clave_memoria = ftok(clave, 'A');
    if (clave_memoria == -1) {
        perror("surtidor: ftok");
        exit(EXIT_FAILURE);
    }

    //cola de mensajes creada por el gestor
    int id_cola = msgget(clave_memoria, 0666);
    if (id_cola == -1) {
        perror("surtidor: msgget");
        exit(EXIT_FAILURE);
    }

    //memoria compartida
    int id_memoria = shmget(clave_memoria, sizeof(int), 0666);
    if (id_memoria == -1) {
        perror("surtidor: shmget");
        exit(EXIT_FAILURE);
    }

    int *capacidad_surtidor = (int *)shmat(id_memoria, NULL, 0);
    if (capacidad_surtidor == (void *)-1) {
        perror("surtidor: shmat");
        exit(EXIT_FAILURE);
    }

    /* grupo de semáforos:
       0 -> depósito proveedor
       1 -> mutex capacidad_surtidor */
    int id_semaforo = semget(clave_memoria, 2, 0666);
    if (id_semaforo == -1) {
        perror("surtidor: semget");
        exit(EXIT_FAILURE);
    }

    struct sembuf op;
    struct mensaje msg;
    struct fluido f;
    int contador_descargas = 0;

    while (1) {
        sleep(periodo);

        // intentar restar 'volumen' litros del depósito sin bloquear 
        op.sem_num = 0;          // semáforo del depósito
        op.sem_op  = -volumen;   // restar volumen 
        op.sem_flg = IPC_NOWAIT; // de esta forma no es bloqueante

        if (semop(id_semaforo, &op, 1) == -1) {
            if (errno == EAGAIN) {
                // no hay volumen suficiente → mensaje de alerta tipo 1 
                msg.tipo = 1;
                msg.pid  = getpid();
                snprintf(msg.texto, sizeof(msg.texto),"Problema de suministro en el surtidor %d, caudal insuficiente",msg.pid);

                if (msgsnd(id_cola,&msg,sizeof(struct mensaje) - sizeof(long),0) == -1) {
                    perror("surtidor: msgsnd alerta");
                }
                continue;
            } else {
                perror("surtidor: semop deposito");
                exit(EXIT_FAILURE);
            }
        }

        //conseguido el volumen
        contador_descargas++;
        f.contador = contador_descargas;
        f.caudal   = volumen;

        // enviar struct fluido por stdout (hacia caudalimetro)
        if (fwrite(&f, sizeof(struct fluido), 1, stdout) != 1) {
            perror("surtidor: fwrite fluido");
        }
        fflush(stdout);

        // actualizar capacidad del surtidor  (semáforo 1) 
        op.sem_num = 1;
        op.sem_op  = -1; //intenta restar -1
        op.sem_flg = 0; //es una operacion bloqueante

        if (semop(id_semaforo, &op, 1) == -1) { //si ya esta en 0 se detiene aqui hasta que se libere
            perror("surtidor: semop down mutex");
            exit(EXIT_FAILURE);
        }

        // sección crítica 
        *capacidad_surtidor += volumen;

        //libera el semaforo
        op.sem_op = 1;
        if (semop(id_semaforo, &op, 1) == -1) {
            perror("surtidor: semop up mutex");
            exit(EXIT_FAILURE);
        }
    }

    // no se llegara normalmente
    shmdt(capacidad_surtidor);
    return 0;
}
