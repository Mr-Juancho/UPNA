#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// La misma estructura de mensaje
typedef struct mensaje {
    int secuencia;
    int pidEmisor;
} Mensaje;

int main() {
    char *fifo_ida = "fifo_ida";
    char *fifo_vuelta = "fifo_vuelta";
    int fd_ida, fd_vuelta;
    Mensaje msg;

    printf("Receptor (PID %d) esperando al emisor...\n", getpid());

    // 1. Abrir las FIFOs
    // Abrimos la de ida para leer. Se bloqueará hasta que el emisor la abra para escribir.
    fd_ida = open(fifo_ida, O_RDONLY);
    // Abrimos la de vuelta para escribir. Se bloqueará hasta que el emisor la abra para leer.
    fd_vuelta = open(fifo_vuelta, O_WRONLY);
    printf("¡Emisor conectado!\n\n");

    // 2. Bucle de comunicación
    for (int i = 0; i < 10; i++) {
        // Leer mensaje
        read(fd_ida, &msg, sizeof(Mensaje));
        printf("Receptor (PID %d) recibe: secuencia %d de PID %d\n", getpid(), msg.secuencia, msg.pidEmisor);

        // Modificar mensaje
        msg.secuencia++;
        msg.pidEmisor = getpid();

        // Enviar respuesta
        write(fd_vuelta, &msg, sizeof(Mensaje));
    }

    // 3. Cerrar
    close(fd_ida);
    close(fd_vuelta);

    printf("Receptor termina.\n");
    return 0;
}