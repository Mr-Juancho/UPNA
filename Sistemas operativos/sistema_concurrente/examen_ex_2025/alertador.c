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

struct mensajeAlerta {
    long tipo;
    char texto[100];
};


// ./alertador clave [ficherolog]
int main(int argc, char *argv[]) {

    if (argc < 2){
        fprintf(stderr, "Numero de argumentos invalido\n");
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    char ficheroLog[100] ="";
    if(argc == 3){
        strcpy(ficheroLog, argv[2]);
    }
    
    //misma clave que usa meteoestacion
    key_t clave_memoria = ftok(clave, 'A');
    if (clave_memoria == -1) {
        perror("surtidor: ftok");
        exit(EXIT_FAILURE);
    }
    int id_cola = msgget(clave_memoria, 0666);
    if (id_cola == -1){
        perror("Error al crear la cola de mensajes");
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
    int pipe1[2];
    if(pipe(pipe1) == -1){
        perror("Error al crear pipes");
        exit(EXIT_FAILURE);
    }

    pid_t pid_impresora;
    pid_impresora = fork();

    if (pid_impresora == -1){
        perror("Error en la creación del proceso anemometro");
        exit (EXIT_FAILURE);
    } else if (pid_impresora == 0){
        if(dup2(pipe1[0],STDIN_FILENO) == -1){
            perror("dup2 alertador impresora");
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]);
        close(pipe1[1]);
        struct mensajeAlerta msg_leido;
        
        if(argc == 3){
            int fd = open(ficheroLog, O_CREAT | O_WRONLY | O_TRUNC, 0666);
            if (fd == -1) {
                perror("impresora: open ficheroLog");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("impresora: dup2 stdout ficheroLog");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);
            
        }
        char linea[256];

        while (fgets(linea, sizeof(linea), stdin) != NULL) {
            int palabras = contar_palabras(linea);

            // Decrementar tinta (sem 0) en 'palabras' SIN BLOQUEAR
            struct sembuf op = { .sem_num = 0, .sem_op = -palabras, .sem_flg = IPC_NOWAIT };
            if (semop(id_semaforo, &op, 1) == -1) {
                // Si no hay suficiente tinta, muere
                if (errno == EAGAIN) {
                    fprintf(stderr, "impresora: ERROR, no hay tinta suficiente. Muero.\n");
                } else {
                    perror("impresora: semop tinta");
                }
                // al salir se cerrará la tubería/archivo por el SO
                exit(EXIT_FAILURE);
            }

            // Hay tinta: imprime el mensaje completo
            fputs(linea, stdout);
            fflush(stdout);
        }

        // Si se cerró la tubería, termina
        exit(EXIT_SUCCESS);
    }

    //para el proceso alertador
    if(dup2(pipe1[1],STDOUT_FILENO) == -1){
            perror("dup2 alertador");
            exit(EXIT_FAILURE);
    }
    close(pipe1[0]);
    close(pipe1[1]);

    struct mensajeAlerta msg;

    while(1){

        if(msgrcv(id_cola, &msg, sizeof(stuct mensajeAlerta) -sizeof(long),-2,0) == -1){
            perror("Error alertador cola mensajes recibidos");
            exit(EXIT_FAILURE);
        }

        printf("%s", msg.texto);
        fflush(stdout);
    }


    return 0;
}
