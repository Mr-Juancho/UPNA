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
#include <sys/stat.h>
#include <errno.h>

//
// En Linux normalmente hay que definir union semun.
// En macOS a veces ya viene definida; si te da "redefinition", protégelo con #if.
// (mínimo cambio: lo dejo tal cual, pero con el campo correcto "val")
//
union semun {
    int val;                    // SETVAL
    struct semid_ds *buf;       // IPC_STAT, IPC_SET
    unsigned short *array;      // SETALL
};

volatile sig_atomic_t salida = 0;

void handler(int sig) {
    (void)sig;
    salida = 1;
}

// Mensaje de pedidos (según enunciado deberían ir más campos, pero lo dejo como tú lo tienes)
struct mensaje {
    long tipo;
    int cantGalletas;
    int cantBatidos;
    int identificador;
};

// Helpers P/V para semáforos (mínimo cambio y te evita repetir código)
static void sem_P(int semid, unsigned short num) {
    struct sembuf op = { .sem_num = num, .sem_op = -1, .sem_flg = 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("semop P");
        exit(EXIT_FAILURE);
    }
}

static void sem_V(int semid, unsigned short num) {
    struct sembuf op = { .sem_num = num, .sem_op = +1, .sem_flg = 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("semop V");
        exit(EXIT_FAILURE);
    }
}

// ./gestor clave aforo numCamareros nombreFIFO
int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s clave aforo numCamareros nombreFIFO\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int aforo = atoi(argv[2]);
    int numCamareros = atoi(argv[3]);
    char nombreFIFO[256];
    strcpy(nombreFIFO, argv[4]);

    key_t clave_ipc = ftok(clave, 'A');
    if (clave_ipc == -1) {
        perror("gestor: ftok");
        exit(EXIT_FAILURE);
    }

    // 1) Cola de mensajes (pedidos)
    int id_cola = msgget(clave_ipc, 0666 | IPC_CREAT);
    if (id_cola == -1) {
        perror("gestor: msgget");
        exit(EXIT_FAILURE);
    }

    // 2) Memoria compartida: 3 enteros (pedidos atendidos, galletas servidas, batidos servidos)
    int id_memoria = shmget(clave_ipc, 3 * sizeof(int), 0666 | IPC_CREAT);
    if (id_memoria == -1) {
        perror("gestor: shmget");
        exit(EXIT_FAILURE);
    }

    int *stats = (int *)shmat(id_memoria, NULL, 0);
    if (stats == (void *)-1) {
        perror("gestor: shmat");
        exit(EXIT_FAILURE);
    }

    // inicializa a 0
    for (int k = 0; k < 3; k++) stats[k] = 0;

    // 3) Semáforos (2):
    //    sem 0 -> aforo del local (inicial = aforo)
    //    sem 1 -> mutex para proteger stats (inicial = 1)
    int id_semaforo = semget(clave_ipc, 2, 0666 | IPC_CREAT);
    if (id_semaforo == -1) {
        perror("gestor: semget");
        exit(EXIT_FAILURE);
    }

    union semun arg;
    arg.val = aforo;
    if (semctl(id_semaforo, 0, SETVAL, arg) == -1) {
        perror("gestor: semctl SETVAL aforo");
        exit(EXIT_FAILURE);
    }
    arg.val = 1;
    if (semctl(id_semaforo, 1, SETVAL, arg) == -1) {
        perror("gestor: semctl SETVAL mutex");
        exit(EXIT_FAILURE);
    }

    // 4) FIFO nombreFIFO para comunicar camareros con cocina
    if (mkfifo(nombreFIFO, 0666) == -1) {
        if (errno != EEXIST) { // si ya existe, lo aceptamos
            perror("gestor: mkfifo");
            exit(EXIT_FAILURE);
        }
    }

    // 5) Crear procesos camarero
    pid_t pids_camareros[numCamareros];

    for (int i = 0; i < numCamareros; i++) {
        pid_t pid_camarero = fork();
        if (pid_camarero == -1) {
            perror("gestor: fork camarero");
            exit(EXIT_FAILURE);
        } else if (pid_camarero == 0) {
            // ./camarero clave nombreFIFO  (mínimo cambio, tú ya lo estabas pasando así)
            execl("./camarero", "camarero", clave, nombreFIFO, (char *)NULL);
            perror("gestor: execl camarero");
            exit(EXIT_FAILURE);
        } else {
            pids_camareros[i] = pid_camarero;
        }
    }

    // 6) Crear proceso cocina
    pid_t pid_cocina = fork();
    if (pid_cocina == -1) {
        perror("gestor: fork cocina");
        exit(EXIT_FAILURE);
    } else if (pid_cocina == 0) {
        // ./cocina clave nombreFIFO (mínimo cambio: pasamos lo que realmente tenemos)
        execl("./cocina", "cocina", clave, nombreFIFO, (char *)NULL);
        perror("gestor: execl cocina");
        exit(EXIT_FAILURE);
    }

    // 7) Capturar SIGINT para salida ordenada
    signal(SIGINT, handler);

    printf("----Gestor en ejecución----- (CTRL+C para terminar)\n");

    // 8) Cada 5s imprimir reporte usando mutex (sem 1) sin esperas activas
    while (!salida) {
        sleep(5);

        // entra en sección crítica: stats
        sem_P(id_semaforo, 1);

        int pedidos_atendidos = stats[0];
        int galletas_servidas = stats[1];
        int batidos_servidos  = stats[2];

        sem_V(id_semaforo, 1);

        printf("[REPORTE] pedidos=%d, galletas=%d, batidos=%d\n",
               pedidos_atendidos, galletas_servidas, batidos_servidos);
        fflush(stdout);
    }

    // 9) Terminación ordenada (reenviar SIGINT, esperar, liberar recursos)
    printf("Recibido SIGINT, terminando ordenadamente...\n");

    // reenviar SIGINT a camareros
    for (int i = 0; i < numCamareros; i++) {
        kill(pids_camareros[i], SIGINT);
    }
    // reenviar SIGINT a cocina
    kill(pid_cocina, SIGINT);

    // esperar a que terminen
    for (int i = 0; i < numCamareros; i++) {
        waitpid(pids_camareros[i], NULL, 0);
    }
    waitpid(pid_cocina, NULL, 0);

    // destruir recursos IPC
    if (shmdt(stats) == -1)
        perror("gestor: shmdt");
    if (shmctl(id_memoria, IPC_RMID, NULL) == -1)
        perror("gestor: shmctl IPC_RMID");

    if (msgctl(id_cola, IPC_RMID, NULL) == -1)
        perror("gestor: msgctl IPC_RMID");

    if (semctl(id_semaforo, 0, IPC_RMID) == -1)
        perror("gestor: semctl IPC_RMID");

    // eliminar FIFO
    if (unlink(nombreFIFO) == -1) {
        perror("gestor: unlink FIFO");
    }

    printf("Gestor terminado correctamente. ¡Hasta luego!\n");
    return 0;
}
