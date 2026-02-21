#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define FIFO_NAME "mi_fifo"

int main() {
    int fd;
    char mensaje[] = "Hola desde el otro programa!";

    // Abre la FIFO solo para escribir.
    // Esta llamada se quedará BLOQUEADA hasta que un lector la abra.
    fd = open(FIFO_NAME, O_WRONLY);

    printf("Escritor: He conseguido un lector. Enviando mensaje...\n");
    
    // Escribe el mensaje en la FIFO
    write(fd, mensaje, strlen(mensaje) + 1);

    // Cierra su extremo de la tubería
    close(fd);

    return 0;
}