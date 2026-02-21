#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#define FIFO_NAME "mi_fifo"

int main() {
    int fd;
    char buffer[100];

    // Abre la FIFO solo para leer.
    // Esta llamada se quedará BLOQUEADA hasta que un escritor la abra.
    fd = open(FIFO_NAME, O_RDONLY);

    printf("Lector: Hay un escritor. Esperando mensaje...\n");

    // Lee de la FIFO y lo guarda en el buffer
    read(fd, buffer, sizeof(buffer));

    printf("Lector: Mensaje recibido: '%s'\n", buffer);

    // Cierra su extremo de la tubería
    close(fd);

    return 0;
}