// cocina.c
// Uso: ./cocina clave nombreFIFO
//
// Requisitos del enunciado:
// - Lee en bucle de forma BLOQUEANTE de la FIFO 'nombreFIFO'.
// - Espera recibir mensajes con formato: "entero espacio entero\n"
//   => pidCliente y numProductos (total).
// - Espera numProductos segundos (simula preparación).
// - Envía SIGUSR1 al pidCliente para indicar que el pedido está listo.
// - Termina ordenadamente al recibir SIGINT (presumiblemente enviado por gestor).
//
// Señales: versión SIMPLE -> solo signal() + handler.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

static volatile sig_atomic_t salir = 0;

void handler(int sig) {
    (void)sig;
    salir = 1;
}

// ./cocina clave nombreFIFO
int main(int argc, char *argv[]) {
    (void)argv; // clave no se usa aquí, pero lo mantenemos por interfaz del proyecto
    if (argc < 3) {
        fprintf(stderr, "Uso: %s clave nombreFIFO\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *nombreFIFO = argv[2];

    // Señal simple para terminar
    signal(SIGINT, handler);

    // Abrimos la FIFO en modo lectura (bloqueante) usando stdio (fgets)
    // Esto se bloqueará hasta que algún camarero la abra para escribir.
    FILE *fifo = fopen(nombreFIFO, "r");
    if (!fifo) {
        perror("cocina: fopen FIFO");
        exit(EXIT_FAILURE);
    }

    printf("[cocina %d] lista. Leyendo de FIFO=%s\n", getpid(), nombreFIFO);

    char linea[128];

    while (!salir) {
        // Lectura bloqueante: espera línea "pid numProductos\n"
        if (fgets(linea, sizeof(linea), fifo) == NULL) {
            if (salir) break;

            // Si el escritor cerró la FIFO, fgets puede devolver NULL (EOF).
            // Para seguir atendiendo pedidos, reabrimos la FIFO y continuamos.
            if (feof(fifo)) {
                clearerr(fifo);
                fclose(fifo);

                fifo = fopen(nombreFIFO, "r");
                if (!fifo) {
                    perror("cocina: reopen FIFO");
                    break;
                }
                continue;
            }

            // Si fue un error real
            if (ferror(fifo)) {
                perror("cocina: fgets FIFO");
                break;
            }

            continue;
        }

        // Parsear "pid numProductos"
        int pidCliente = 0;
        int numProductos = 0;

        if (sscanf(linea, "%d %d", &pidCliente, &numProductos) != 2) {
            fprintf(stderr, "[cocina] linea invalida: %s", linea);
            continue;
        }

        if (numProductos < 0) numProductos = 0;

        // Simular preparación
        printf("[cocina] Pedido de pid=%d, productos=%d -> preparando...\n", pidCliente, numProductos);
        fflush(stdout);

        // sleep() puede ser interrumpido por SIGINT; lo manejamos de forma simple
        for (int i = 0; i < numProductos && !salir; i++) {
            sleep(1);
        }

        if (salir) break;

        // Avisar al cliente
        if (kill((pid_t)pidCliente, SIGUSR1) == -1) {
            perror("[cocina] kill(SIGUSR1) al cliente");
        } else {
            printf("[cocina] Pedido listo -> SIGUSR1 enviado a pid=%d\n", pidCliente);
            fflush(stdout);
        }
    }

    printf("[cocina %d] SIGINT recibido, terminando ordenadamente...\n", getpid());
    fclose(fifo);
    return 0;
}
