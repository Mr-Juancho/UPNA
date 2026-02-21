#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h> // Necesario para wait()

// Se mantiene tu estructura original
typedef struct mensaje {
    int secuencia;
    int pidEmisor;
} Mensaje;

int main(int argc, char *argv[]) {

    // --- CAMBIO 1: Se necesitan DOS tuberías ---
    // Una para que el padre escriba al hijo (ida)
    // Otra para que el hijo escriba al padre (vuelta)
    int tuberia_ida[2];
    int tuberia_vuelta[2];
    pid_t pid;
    Mensaje msg;

    // Creamos ambas tuberías
    if (pipe(tuberia_ida) == -1 || pipe(tuberia_vuelta) == -1) {
        fprintf(stderr, "Error al crear las tuberias\n");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Error al crear el proceso hijo\n");
        return 2;
    }
    // --- FIN CAMBIO 1 ---

    if (pid == 0) { // Lógica del Proceso HIJO
        // --- CAMBIO 2: Cerrar los extremos que NO se usan ---
        close(tuberia_ida[1]);      // El hijo no escribe en la tubería de ida
        close(tuberia_vuelta[0]);   // El hijo no lee de la tubería de vuelta

        // --- CAMBIO 3: Bucle para 10 mensajes ---
        for (int i = 0; i < 10; i++) {
            // 1. Leer el mensaje del padre
            read(tuberia_ida[0], &msg, sizeof(Mensaje));
            printf("Hijo (PID %d) recibe: secuencia %d de PID %d\n", getpid(), msg.secuencia, msg.pidEmisor);

            // 2. Modificar el mensaje
            msg.secuencia++;
            msg.pidEmisor = getpid();

            // 3. Devolver el mensaje al padre
            write(tuberia_vuelta[1], &msg, sizeof(Mensaje));
        }

        // --- CAMBIO 4: Cerrar los extremos usados y salir ---
        close(tuberia_ida[0]);
        close(tuberia_vuelta[1]);
        printf("Hijo termina.\n");
        exit(0);

    } else { // Lógica del Proceso PADRE
        // --- CAMBIO 2: Cerrar los extremos que NO se usan ---
        close(tuberia_ida[0]);      // El padre no lee de la tubería de ida
        close(tuberia_vuelta[1]);   // El padre no escribe en la tubería de vuelta

        // Preparar el primer mensaje
        msg.secuencia = 1;
        msg.pidEmisor = getpid();

        // --- CAMBIO 3: Bucle para 10 mensajes ---
        for (int i = 0; i < 10; i++) {
            // 1. Enviar mensaje al hijo
            printf("Padre (PID %d) envía: secuencia %d\n", getpid(), msg.secuencia);
            write(tuberia_ida[1], &msg, sizeof(Mensaje));

            // 2. Esperar y leer la respuesta del hijo
            read(tuberia_vuelta[0], &msg, sizeof(Mensaje));
            printf("Padre (PID %d) recibe respuesta: secuencia %d de PID %d\n\n", getpid(), msg.secuencia, msg.pidEmisor);
            
            // 3. Preparar el siguiente mensaje (el hijo ya incrementó la secuencia)
            msg.pidEmisor = getpid();
        }

        // --- CAMBIO 4: Cerrar los extremos usados ---
        close(tuberia_ida[1]);
        close(tuberia_vuelta[0]);

        // --- CAMBIO 5: Esperar a que el hijo termine ---
        wait(NULL);
        printf("Padre termina.\n");
    }

    return 0;
}