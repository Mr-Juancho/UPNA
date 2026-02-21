// camarero.c
// Uso: ./camarero clave nombreFIFO
//
// Requisitos del enunciado (resumen):
// - Desencola pedidos con prioridad: tipo 2 (domicilio) > tipo 1 (local).
// - Si no hay pedidos, espera NO activa (bloqueante).
// - Al atender un pedido, actualiza estadísticas en memoria compartida protegida por semáforo.
//   (El enunciado pide esperar 5 segundos ANTES de liberar el semáforo).
// - Crea un recibo llamado PID.txt (PID del cliente).
// - Escribe en la FIFO: "pidDelCliente totalProductos\n" para que lo lea cocina.
// - Termina ordenadamente al recibir SIGINT (enviado por gestor).
//
// Señales: versión SIMPLE -> solo signal() + handler.

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
#include <errno.h>
#include <fcntl.h>

// Mensaje de pedido en la cola (mtype = tipo: 1 local, 2 domicilio)
struct mensaje {
    long tipo;            // 1=local, 2=domicilio (mtype SysV)
    int cantGalletas;
    int cantBatidos;
    int identificador;    // PID del cliente
};

static volatile sig_atomic_t salir = 0;

void handler(int sig) {
    (void)sig;
    salir = 1;
}

// Semáforos P/V (sem 1 = mutex stats)
static void sem_P(int semid, unsigned short num) {
    struct sembuf op = { .sem_num = num, .sem_op = -1, .sem_flg = 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("camarero: semop P");
        exit(EXIT_FAILURE);
    }
}
static void sem_V(int semid, unsigned short num) {
    struct sembuf op = { .sem_num = num, .sem_op = +1, .sem_flg = 0 };
    if (semop(semid, &op, 1) == -1) {
        perror("camarero: semop V");
        exit(EXIT_FAILURE);
    }
}

// Escribe recibo PID.txt
static void escribir_recibo(const struct mensaje *m) {
    char nombre[64];
    snprintf(nombre, sizeof(nombre), "%d.txt", m->identificador);

    FILE *f = fopen(nombre, "w");
    if (!f) {
        perror("camarero: fopen recibo");
        return;
    }

    fprintf(f, "RECIBO CLIENTE PID=%d\n", m->identificador);
    fprintf(f, "Tipo pedido: %s\n", (m->tipo == 2) ? "DOMICILIO" : "LOCAL");
    fprintf(f, "Galletas: %d\n", m->cantGalletas);
    fprintf(f, "Batidos : %d\n", m->cantBatidos);
    fprintf(f, "Total productos: %d\n", m->cantGalletas + m->cantBatidos);
    fclose(f);
}

// Envía a FIFO: "pid total\n"
static void enviar_a_fifo(const char *fifo, int pidCliente, int totalProductos) {
    // O_WRONLY puede bloquear hasta que cocina abra lectura (esto NO es espera activa)
    int fd = open(fifo, O_WRONLY);
    if (fd == -1) {
        perror("camarero: open FIFO");
        return;
    }

    char linea[64];
    int n = snprintf(linea, sizeof(linea), "%d %d\n", pidCliente, totalProductos);
    if (write(fd, linea, (size_t)n) == -1) {
        perror("camarero: write FIFO");
    }
    close(fd);
}

// Procesa un pedido: stats + recibo + fifo
static void procesar_pedido(const char *fifo, int semid, int *stats, const struct mensaje *m) {
    int total = m->cantGalletas + m->cantBatidos;

    // Actualizar estadísticas protegidas por sem 1
    sem_P(semid, 1);
    stats[0] += 1;               // pedidos atendidos
    stats[1] += m->cantGalletas; // galletas servidas
    stats[2] += m->cantBatidos;  // batidos servidos

    // El enunciado pide: esperar 5s ANTES de liberar el semáforo
    sleep(5);

    sem_V(semid, 1);

    // Crear recibo PID.txt
    escribir_recibo(m);

    // Escribir a FIFO "pid total"
    enviar_a_fifo(fifo, m->identificador, total);
}

// ./camarero clave nombreFIFO
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s clave nombreFIFO\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);

    char nombreFIFO[256];
    strcpy(nombreFIFO, argv[2]);

    // Señal simple
    signal(SIGINT, handler);

    key_t k = ftok(clave, 'A');
    if (k == -1) { perror("camarero: ftok"); exit(EXIT_FAILURE); }

    // Cola creada por gestor
    int id_cola = msgget(k, 0666);
    if (id_cola == -1) { perror("camarero: msgget"); exit(EXIT_FAILURE); }

    // Memoria compartida stats (3 ints) creada por gestor
    int id_mem = shmget(k, 3 * sizeof(int), 0666);
    if (id_mem == -1) { perror("camarero: shmget"); exit(EXIT_FAILURE); }

    int *stats = (int *)shmat(id_mem, NULL, 0);
    if (stats == (void *)-1) { perror("camarero: shmat"); exit(EXIT_FAILURE); }

    // Semáforos (2): sem0 aforo, sem1 mutex stats
    int semid = semget(k, 2, 0666);
    if (semid == -1) { perror("camarero: semget"); exit(EXIT_FAILURE); }

    printf("[camarero %d] listo. FIFO=%s\n", getpid(), nombreFIFO);

    while (!salir) {
        struct mensaje msg;

        // PRIORIDAD (simple):
        // 1) Intentar coger tipo 2 SIN bloquear
        // 2) Si no hay tipo 2, bloquear esperando tipo 1
        //
        // Esto NO es espera activa: cuando no hay tipo 2, el proceso queda bloqueado en msgrcv tipo 1.
        // Además, tipo 2 tiene prioridad porque siempre se intenta primero.

        ssize_t r = msgrcv(id_cola, &msg, sizeof(struct mensaje) - sizeof(long), 2, IPC_NOWAIT);
        if (r != -1) {
            procesar_pedido(nombreFIFO, semid, stats, &msg);
            continue;
        }

        // Si no hay tipo 2
        if (errno != ENOMSG) {
            // Si fue interrumpido por SIGINT, salimos
            if (errno == EINTR && salir) break;
            perror("camarero: msgrcv tipo2");
            break;
        }

        // Esperar tipo 1 (bloqueante)
        r = msgrcv(id_cola, &msg, sizeof(struct mensaje) - sizeof(long), 1, 0);
        if (r == -1) {
            if (errno == EINTR && salir) break;
            perror("camarero: msgrcv tipo1");
            break;
        }

        procesar_pedido(nombreFIFO, semid, stats, &msg);
    }

    printf("[camarero %d] SIGINT recibido, terminando ordenadamente...\n", getpid());
    shmdt(stats);
    return 0;
}
