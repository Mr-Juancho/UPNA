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

volatile sig_atomic_t recibido = 0;
volatile sig_atomic_t salida = 0;

void handler(int sig) {
    (void)sig;
    recibido = 1;
}
void handler2(int sig) {
    (void)sig;
    salida = 1;
}

struct mensajeAlerta {
    long tipo;
    char texto[100];
}; 

int main(int argc, char *argv[]) {

    if (argc < 7){
        fprintf(stderr, "numero de argumentos invalido\n");
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int numTermo = atoi(argv[2]);
    int periodoTermo = atoi(argv[3]);
    int umbralTermo  = atoi(argv[4]);
    int periodoAnem  = atoi(argv[5]);
    int umbralAnem  = atoi(argv[6]);

    char ficheroLog[100] = "";
    if (argc == 8){
        strcpy(ficheroLog, argv[7]);
    }

    printf("la clave es %s\n", clave);
    
    key_t clave_memoria = ftok(clave, 'A');
    if (clave_memoria == -1) {
        perror("Error en ftok");
        exit(EXIT_FAILURE);
    }

    // cola de mensajes 
    int id_cola = msgget(clave_memoria, 0666 | IPC_CREAT);
    if (id_cola == -1){
        perror("Error al crear la cola de mensajes");
        exit(EXIT_FAILURE);
    }

    // memoria compartida: capacidad del surtidor 
    int id_memoria = shmget(clave_memoria, numTermo* sizeof(int), 0666 | IPC_CREAT);
    if (id_memoria == -1){
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }
    int *temps = shmat(id_memoria, NULL, 0);
    if (temps == (void *)-1) {
        perror("Error al vincular la memoria compartida");
        exit(EXIT_FAILURE);
    }

    for (int k=0;k<numTermo;k++){
        temps[k] = 0;
    }

    /* grupo de 2 semáforos:
       0 -> deposito tinta
       1 -> mutex memoria compartida */
    int id_semaforo = semget(clave_memoria, 2, 0666 | IPC_CREAT);
    if (id_semaforo == -1){
        perror("Error al crear o acceder al semaforo");
        exit(EXIT_FAILURE);
    }
    union semun arg;

    // semáforo 0: volumen depósito = 0 litros
    arg.val = 100; //signfica que el recurso empieza en 100
    if (semctl(id_semaforo, 0, SETVAL, arg) == -1) {
        perror("Error init sem deposito");
        exit(EXIT_FAILURE);
    }

    // semáforo 1: mutex memoria = 1 
    arg.val = 1; //signfica que esta libre
    if (semctl(id_semaforo, 1, SETVAL, arg) == -1) {
        perror("Error init sem mutex");
        exit(EXIT_FAILURE);
    }

    //creacion de procesos termometros
    pid_t pid_termometro,pid_anemometro, pid_alertador;

    pid_t pids_termometros[numTermo];
    for (int i= 0; i<numTermo; i++){
        pid_termometro = fork();
        if (pid_termometro == -1){
            perror("Error en la creación del proceso termometro");
            exit (EXIT_FAILURE);
        } else if (pid_termometro == 0){
            char indice[10];
            snprintf(indice, sizeof(indice), "%d", i);
            execl("./termometro", "termometro", clave, indice, argv[3], argv[4], (char *)NULL);
            perror("Error al ejecutar termometro");
            exit(EXIT_FAILURE);
        }else{
            pids_termometros[i] = pid_termometro;
        }

    }
    //anemometro
    pid_anemometro = fork();
    if (pid_anemometro == -1){
        perror("Error en la creación del proceso anemometro");
        exit (EXIT_FAILURE);
    } else if (pid_anemometro == 0){
        execl("./anemometro", "anemometro", clave, argv[5], argv[6], (char *)NULL);
        perror("Error al ejecutar anenometro");
        exit(EXIT_FAILURE);
    }

    //alertador
    pid_alertador = fork();
    if (pid_alertador == -1){
        perror("Error en la creación del proceso alertador");
        exit (EXIT_FAILURE);
    } else if (pid_alertador == 0){
        if (argc == 8){
            execl("./alertador", "alertador", clave, ficheroLog, (char *)NULL);
            perror("Error al ejecutar alertador");
        }else{
            execl("./alertador", "alertador", clave, (char *)NULL);
            perror("Error al ejecutar alertador");
        }
        exit(EXIT_FAILURE);
    }

    signal(SIGUSR1, handler);
    signal(SIGINT,handler2);

    printf("----Meteoestacion en ejecución-----\n");

    while(salida!=1){
        while(!recibido) pause();
        recibido = 0;      

        struct sembuf op = {.sem_num = 1, .sem_op=-1,.sem_flg=0}; //configuracion del sem 1, espera bloqueante
        semop(id_semaforo, &op, 1); //ejecucion del semaforo

        long suma = 0;
        float tempMedia = 0.0;
        for (int i=0; i<numTermo; i++) suma += temps[i];
        if(numTermo !=0) tempMedia = (float)suma / (float)numTermo;
        op.sem_op = 1;
        if (semop(id_semaforo, &op, 1) == -1) {
            perror("meteoestacion: up mutex");
            exit(EXIT_FAILURE);
        }
        if(tempMedia >umbralTermo){
            struct mensajeAlerta msg;
            msg.tipo = 2;
            snprintf(msg.texto, sizeof(msg.texto),"Alerta de calor con una temperatura media de %.2f grados", tempMedia);
            if(msgsnd(id_cola, &msg, sizeof(msg.texto), 0) == -1) perror("msgsnd meteoestacion");
        }
    }
    
    printf("Recibido SIGINT, limpiando recursos...\n");

    //matamos procesos hijos
    for (int i = 0; i < numTermo; i++) {
        kill(pids_termometros[i], SIGKILL);
        waitpid(pids_termometros[i], NULL, 0);
    }

    kill(pid_anemometro,SIGKILL);
    kill(pid_alertador,SIGKILL);


    //esperamos que termine
    waitpid(pid_anemometro, NULL, 0);
    waitpid(pid_alertador, NULL, 0);

    //liberamos la infraestructura
    if (shmdt(temps) == -1)
        perror("Error al desvincular memoria compartida");
    if (shmctl(id_memoria, IPC_RMID, NULL) == -1)
        perror("Error al borrar memoria compartida");

    if (msgctl(id_cola, IPC_RMID, NULL) == -1)
        perror("Error al borrar cola de mensajes");

    if (semctl(id_semaforo, 0, IPC_RMID) == -1)
        perror("Error al borrar semaforos");

    printf("Meteoestacion terminado correctamente.\n");

    return 0;
}
