#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int main() {
    pid_t pid = fork();

    if (pid == -1) {
        // Error
        perror("fork falló");
        return 1;
    } else if (pid == 0) {
        // --- Proceso HIJO ---
        printf("Hijo: Voy a convertirme en 'ls -l'.\n");
        
        // La 'l' significa que pasamos los argumentos como una lista.
        // La 'p' significa que buscará 'ls' en el PATH del sistema.
        // El primer "ls" es el comando, el segundo es argv[0] (el nombre del programa),
        // y luego vienen los argumentos. La lista DEBE terminar en NULL.
        execlp("ls", "ls", "-l", NULL);
        
        // IMPORTANTE: Si exec tiene éxito, esta línea NUNCA se ejecuta.
        // Si estamos aquí, es porque execlp falló.
        perror("execlp falló");
        return 1; // Salir del hijo si falla
    } else {
        // --- Proceso PADRE ---
        printf("Padre: He creado un hijo con PID %d y estoy esperando a que termine.\n", pid);
        wait(NULL); // Esperar a que el hijo termine
        printf("Padre: Mi hijo ha terminado. Fin del programa.\n");
    }

    return 0;
}