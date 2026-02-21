#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h> // Necesario para comprobar el error EAGAIN

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

    printf("Receptor (PID %d) esperando al emisor...\n", getpid());

    // --- CAMBIO CLAVE 1: Abrir la FIFO de lectura en modo NO BLOQUEANTE ---
    fd_ida = open(fifo_ida, O_RDONLY | O_NONBLOCK);
    fd_vuelta = open(fifo_vuelta, O_WRONLY);

    // Si las FIFOs no existen aún, open puede fallar.
    if (fd_ida == -1) {
        perror("Error al abrir fifo_ida. ¿El emisor se ha ejecutado primero?");
        return 1;
    }
     if (fd_vuelta == -1) {
        perror("Error al abrir fifo_vuelta.");
        return 1;
    }

    printf("¡Emisor conectado!\n\n");

    for (int i = 0; i < 10; i++) {
        // --- CAMBIO CLAVE 2: Bucle para reintentar la lectura ---
        while ((bytesLeidos = read(fd_ida, &msg, sizeof(Mensaje))) <= 0) {
            // Si read devuelve -1 y el error es EAGAIN, significa "no hay datos ahora"
            if (bytesLeidos == -1 && errno == EAGAIN) {
                printf("Receptor: No hay datos todavía. Esperando 1 segundo...\n");
                sleep(1); // Esperar un poco para no saturar la CPU
                continue; // Volver a intentar la lectura
            } else {
                // Si es otro tipo de error, salimos
                perror("Error al leer de la FIFO");
                exit(1);
            }
        }

        // Si salimos del bucle, es porque hemos leído datos correctamente
        printf("Receptor (PID %d) recibe: secuencia %d de PID %d\n", getpid(), msg.secuencia, msg.pidEmisor);

        // Modificar mensaje
        msg.secuencia++;
        msg.pidEmisor = getpid();

        // Enviar respuesta
        write(fd_vuelta, &msg, sizeof(Mensaje));
    }

    close(fd_ida);
    close(fd_vuelta);

    printf("Receptor termina.\n");
    return 0;
}