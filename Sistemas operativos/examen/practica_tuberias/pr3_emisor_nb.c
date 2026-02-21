#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h> // Necesario para comprobar EAGAIN

typedef struct mensaje {
    int secuencia;
    int pidEmisor;
} Mensaje;

int main() {
    char *fifo_ida = "fifo_ida";
    char *fifo_vuelta = "fifo_vuelta";
    int fd_ida, fd_vuelta;
    Mensaje msg;
    ssize_t bytesLeidos;

    // Crear las FIFOs
    mkfifo(fifo_ida, 0666);
    mkfifo(fifo_vuelta, 0666);

    printf("Emisor (PID %d) esperando al receptor...\n", getpid());

    // Abrir la de ida para escribir (bloqueante, espera al receptor)
    fd_ida = open(fifo_ida, O_WRONLY);
    // --- CAMBIO: Abrir la de vuelta para leer en modo NO BLOQUEANTE ---
    fd_vuelta = open(fifo_vuelta, O_RDONLY | O_NONBLOCK);
    
    printf("¡Receptor conectado!\n\n");

    msg.secuencia = 1;
    msg.pidEmisor = getpid();

    for (int i = 0; i < 10; i++) {
        // Enviar mensaje
        printf("Emisor (PID %d) envía: secuencia %d\n", getpid(), msg.secuencia);
        write(fd_ida, &msg, sizeof(Mensaje));

        // Bucle para intentar leer la respuesta
        while ((bytesLeidos = read(fd_vuelta, &msg, sizeof(Mensaje))) <= 0) {
            if (bytesLeidos == -1 && errno == EAGAIN) {
                printf("Emisor: Respuesta no recibida. Reintentando en 1s...\n");
                sleep(1);
                continue;
            } else {
                perror("Error al leer respuesta");
                exit(1);
            }
        }
        
        printf("Emisor (PID %d) recibe respuesta: secuencia %d de PID %d\n\n", getpid(), msg.secuencia, msg.pidEmisor);
        
        msg.pidEmisor = getpid();
    }

    // Cerrar y limpiar
    close(fd_ida);
    close(fd_vuelta);
    unlink(fifo_ida);
    unlink(fifo_vuelta);

    printf("Emisor termina.\n");
    return 0;
}