#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> // Necesario para open()

// La misma estructura de mensaje
typedef struct mensaje {
    int secuencia;
    int pidEmisor;
} Mensaje;

int main() {
    // Nombres para nuestros "buzones" en el sistema de archivos
    char *fifo_ida = "fifo_ida";
    char *fifo_vuelta = "fifo_vuelta";
    int fd_ida, fd_vuelta; // Descriptores de archivo para las FIFOs
    Mensaje msg;

    // 1. Crear las FIFOs (0666 son los permisos)
    mkfifo(fifo_ida, 0666);
    mkfifo(fifo_vuelta, 0666);

    printf("Emisor (PID %d) esperando al receptor...\n", getpid());

    // 2. Abrir las FIFOs
    // Abrimos la de ida para escribir. Se bloqueará hasta que el receptor la abra para leer.
    fd_ida = open(fifo_ida, O_WRONLY);
    // Abrimos la de vuelta para leer. Se bloqueará hasta que el receptor la abra para escribir.
    fd_vuelta = open(fifo_vuelta, O_RDONLY);
    printf("¡Receptor conectado!\n\n");

    // Preparar el primer mensaje
    msg.secuencia = 1;
    msg.pidEmisor = getpid();

    // 3. Bucle de comunicación
    for (int i = 0; i < 10; i++) {
        // Enviar mensaje
        printf("Emisor (PID %d) envía: secuencia %d\n", getpid(), msg.secuencia);
        write(fd_ida, &msg, sizeof(Mensaje));

        // Leer respuesta
        read(fd_vuelta, &msg, sizeof(Mensaje));
        printf("Emisor (PID %d) recibe respuesta: secuencia %d de PID %d\n\n", getpid(), msg.secuencia, msg.pidEmisor);

        // Preparar siguiente mensaje
        msg.pidEmisor = getpid();
    }

    // 4. Cerrar y limpiar
    close(fd_ida);
    close(fd_vuelta);
    unlink(fifo_ida);      // Borra el archivo FIFO
    unlink(fifo_vuelta);   // Borra el otro archivo FIFO

    printf("Emisor termina.\n");
    return 0;
}