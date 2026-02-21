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

void handler(int sig) {
    (void)sig;   
}

int main(int argc, char *argv[]) {

    if (argc < 5){
        fprintf(stderr, "numero de argumentos invalido\n");
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int periodo = atoi(argv[2]);
    int volumen = atoi(argv[3]);
    int umbral  = atoi(argv[4]);

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
    int id_memoria = shmget(clave_memoria, sizeof(int), 0666 | IPC_CREAT);
    if (id_memoria == -1){
        perror("Error al crear la memoria compartida");
        exit(EXIT_FAILURE);
    }
    int *num_litros = (int *)shmat(id_memoria, NULL, 0);
    if (num_litros == (void *)-1) {
        perror("Error al vincular la memoria compartida");
        exit(EXIT_FAILURE);
    }
    *num_litros = 0;
    
    /* grupo de 2 semáforos:
       0 -> depósito proveedor
       1 -> mutex memoria compartida */
    int id_semaforo = semget(clave_memoria, 2, 0666 | IPC_CREAT);
    if (id_semaforo == -1){
        perror("Error al crear o acceder al semaforo");
        exit(EXIT_FAILURE);
    }
    union semun arg;

    // semáforo 0: volumen depósito = 0 litros
    arg.val = 0; //signfica que el recurso empieza vacio , no disponible
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

    // crear tuberías 
    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1){
        perror("Error al crear pipes");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handler);

    // creación de procesos 
    pid_t pid_surtidor, pid_caudalimetro, pid_monitor, pid_sumidero;

    //surtidor
    pid_surtidor = fork();
    if (pid_surtidor == -1){
        perror("Error en la creación del proceso surtidor");
        exit (EXIT_FAILURE);
    } else if (pid_surtidor == 0){

        if (dup2(pipe1[1], STDOUT_FILENO) == -1){ //para que lo lea caudalimetro
            perror("dup2 surtidor");
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        execl("./surtidor", "surtidor", clave, argv[2], argv[3], (char *)NULL);
        perror("Error al ejecutar surtidor");
        exit(EXIT_FAILURE);
    }

    // caudalimetro
    if ((pid_caudalimetro = fork()) == -1){
        perror("Error en la creación del proceso caudalimetro");
        exit (EXIT_FAILURE);
    } else if (pid_caudalimetro == 0){

        if (dup2(pipe1[0], STDIN_FILENO) == -1){ //lectura de surtidor
            perror("dup2 caudalimetro stdin");
            exit(EXIT_FAILURE);
        }
        if (dup2(pipe2[1], STDOUT_FILENO) == -1){
            perror("dup2 caudalimetro stdout");  //salida hacia sumidero
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        execl("./caudalimetro", "caudalimetro", clave, argv[4], (char *)NULL);
        perror("Error al ejecutar caudalimetro");
        exit(EXIT_FAILURE);
    }

    //monitor
    if ((pid_monitor = fork()) == -1){
        perror("Error en la creación del proceso monitor");
        exit (EXIT_FAILURE);
    } else if (pid_monitor == 0){
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        execl("./monitor", "monitor", clave, (char *)NULL);
        perror("Error al ejecutar monitor");
        exit(EXIT_FAILURE);
    }

    // sumidero
    char pid_monitor_str[20];
    snprintf(pid_monitor_str, sizeof(pid_monitor_str), "%d", (int)pid_monitor);

    if ((pid_sumidero = fork()) == -1){
        perror("Error en la creación del proceso sumidero");
        exit (EXIT_FAILURE);
    } else if (pid_sumidero == 0){
        if (dup2(pipe2[0], STDIN_FILENO) == -1){ //lectura de caudalimetro
            perror("Error en dup2 sumidero");
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        execl("./sumidero", "sumidero", clave, pid_monitor_str, (char *)NULL);
        perror("Error al ejecutar sumidero");
        exit(EXIT_FAILURE);
    }

    //cerramos los pipes
    close(pipe1[0]); close(pipe1[1]);
    close(pipe2[0]); close(pipe2[1]);

    printf("Gestor en ejecución (periodo=%d, volumen=%d, umbral=%d). Pulsa Ctrl+C para terminar.\n",
           periodo, volumen, umbral);

    //esperamos ctrl + c
    pause();

    printf("Recibido SIGINT, limpiando recursos...\n");

    //matamos procesos hijos
    kill(pid_surtidor,SIGKILL);
    kill(pid_caudalimetro,SIGKILL);
    kill(pid_monitor,SIGKILL);
    kill(pid_sumidero,SIGKILL);

    //esperamos que termine
    waitpid(pid_surtidor,NULL, 0);
    waitpid(pid_caudalimetro, NULL, 0);
    waitpid(pid_monitor, NULL, 0);
    waitpid(pid_sumidero, NULL, 0);

    //liberamos la infraestructura
    if (shmdt(num_litros) == -1)
        perror("Error al desvincular memoria compartida");
    if (shmctl(id_memoria, IPC_RMID, NULL) == -1)
        perror("Error al borrar memoria compartida");

    if (msgctl(id_cola, IPC_RMID, NULL) == -1)
        perror("Error al borrar cola de mensajes");

    if (semctl(id_semaforo, 0, IPC_RMID) == -1)
        perror("Error al borrar semaforos");

    printf("Gestor terminado correctamente.\n");

    return 0;
}
