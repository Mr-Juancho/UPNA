// cliente.c
// Uso: ./cliente clave ubicacion [numComensales]
//
// Requisitos del enunciado:
// - Se ejecuta en otra terminal.
// - Accede a recursos con la misma 'clave' que gestor.
// - ubicacion: 1 (local) o 2 (domicilio).
// - Si es domicilio (2): encola pedido normalmente.
// - Si es local (1): antes de encolar debe "reservar aforo" restando numComensales del semáforo 0.
//   (operación bloqueante, sin espera activa).
// - Tras encolar, queda latente (pause) esperando SIGUSR1 de cocina.
// - Al recibir SIGUSR1:
//     - si era local: devuelve numComensales al semáforo de aforo (sem 0) y se despide.
//     - si era domicilio: solo se despide.
//
// Señales: versión SIMPLE -> solo signal() + handler.

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

struct mensaje {
    long tipo;            // 1=local, 2=domicilio (mtype SysV)
    int cantGalletas;
    int cantBatidos;
    int identificador;    // PID del cliente
};

static volatile sig_atomic_t listo = 0;

void handler(int sig) {
    (void)sig;
    listo = 1;
}

// Operación sobre semáforo: suma/resta "delta" (bloqueante)
static void sem_add(int semid, unsigned short num, int delta) {
    struct sembuf op;
    op.sem_num = num;
    op.sem_op  = delta;   // negativo resta, positivo suma
    op.sem_flg = 0;       // bloqueante (sin espera activa)

    if (semop(semid, &op, 1) == -1) {
        perror("cliente: semop");
        exit(EXIT_FAILURE);
    }
}

// ./cliente clave ubicacion [numComensales]
int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s clave ubicacion [numComensales]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);

    int ubicacion = atoi(argv[2]); // 1 local, 2 domicilio
    int numComensales = 1;

    if (ubicacion == 1) {
        // Si es local, numComensales es obligatorio o asumimos 1 (según el enunciado lo deja opcional)
        if (argc >= 4) numComensales = atoi(argv[3]);
        if (numComensales <= 0) numComensales = 1;
    }

    // Señal: cocina avisa con SIGUSR1
    signal(SIGUSR1, handler);

    // IPC key
    key_t k = ftok(clave, 'A');
    if (k == -1) { perror("cliente: ftok"); exit(EXIT_FAILURE); }

    // Cola creada por gestor
    int id_cola = msgget(k, 0666);
    if (id_cola == -1) { perror("cliente: msgget"); exit(EXIT_FAILURE); }

    // Semáforos creados por gestor (sem0=aforo, sem1=mutex stats)
    int semid = semget(k, 2, 0666);
    if (semid == -1) { perror("cliente: semget"); exit(EXIT_FAILURE); }

    // Generar pedido aleatorio simple (si tu enunciado dice otra cosa, ajustas aquí)
    srand((unsigned int)(time(NULL) ^ getpid()));
    int galletas = rand() % 6;   // 0..5
    int batidos  = rand() % 4;   // 0..3

    struct mensaje m;
    m.tipo = (ubicacion == 2) ? 2 : 1;
    m.cantGalletas = galletas;
    m.cantBatidos  = batidos;
    m.identificador = (int)getpid();

    // Si es local: reservar aforo (bloqueante)
    if (ubicacion == 1) {
        printf("[cliente %d] LOCAL: reservando aforo para %d comensales...\n", getpid(), numComensales);
        // Restar numComensales del sem 0 (si no hay, se bloquea hasta que haya)
        sem_add(semid, 0, -numComensales);
        printf("[cliente %d] LOCAL: aforo reservado. Encolando pedido...\n", getpid());
    } else {
        printf("[cliente %d] DOMICILIO: encolando pedido...\n", getpid());
    }

    // Encolar pedido
    // Importante: tamaño sin el long tipo
    if (msgsnd(id_cola, &m, sizeof(struct mensaje) - sizeof(long), 0) == -1) {
        perror("cliente: msgsnd");
        // Si falla y era local, devolvemos aforo para no “perder” plazas
        if (ubicacion == 1) sem_add(semid, 0, +numComensales);
        exit(EXIT_FAILURE);
    }

    printf("[cliente %d] Pedido enviado (tipo=%ld, galletas=%d, batidos=%d). Esperando SIGUSR1...\n",
           getpid(), m.tipo, m.cantGalletas, m.cantBatidos);

    // Espera NO activa a SIGUSR1
    while (!listo) pause();

    // Al recibir SIGUSR1, si es local, liberar aforo
    if (ubicacion == 1) {
        printf("[cliente %d] Pedido listo. Liberando aforo (%d comensales)...\n", getpid(), numComensales);
        sem_add(semid, 0, +numComensales);
        printf("[cliente %d] ¡Gracias! Nos vamos del local.\n", getpid());
    } else {
        printf("[cliente %d] Pedido a domicilio listo. ¡Gracias!\n", getpid());
    }

    return 0;
}
