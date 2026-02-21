#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        // Error
        perror("fork falló");
        return 1;
    } else if (pid == 0) {
        // --- Proceso HIJO ---
        printf("Hijo: Voy a convertirme en 'grep main ...'.\n");

        // La 'v' significa que pasamos los argumentos en un vector (array).
        // La 'p' significa que buscará 'grep' en el PATH.
        // El array debe terminar con un puntero NULL.
        char *args[] = {"grep", "main", "ejemplo_execvp.c", NULL};

        execvp(args[0], args);

        // Si exec tiene éxito, esta línea NUNCA se ejecuta.
        perror("execvp falló");
        return 1; // Salir del hijo si falla
    } else {
        // --- Proceso PADRE ---
        printf("Padre: He creado un hijo con PID %d y estoy esperando.\n", pid);
        wait(NULL); // Esperar al hijo
        printf("Padre: Mi hijo ha terminado. Fin del programa.\n");
    }

    /*NULL al final del execvp*/


    return 0;
}