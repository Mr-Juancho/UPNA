#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid_hijo;
    int status;

    // fork() crea un nuevo proceso.
    // El valor que devuelve es la clave para saber quién es quién.
    pid_hijo = fork();

    if (pid_hijo == -1) {
        // Error al crear el proceso
        perror("Error al crear el proceso");
        return 1;
    } else if (pid_hijo == 0) {
        // --- Código del proceso HIJO ---
        // fork() devuelve 0 al proceso hijo.
        printf("Soy el proceso hijo, mi PID es %ld y el de mi padre es %ld.\n", (long)getpid(), (long)getppid());
        sleep(2); // Simulo que el hijo está haciendo alguna tarea
        printf("Hijo: Ya he terminado mi trabajo.\n");
        exit(0); // El hijo termina exitosamente
    } else {
        // --- Código del proceso PADRE ---
        // fork() devuelve el PID del hijo al proceso padre.
        printf("Soy el proceso padre, mi PID es %ld y he creado un hijo con PID %ld.\n", (long)getpid(), (long)pid_hijo);

        printf("Padre: Voy a esperar a que mi hijo específico (PID: %ld) termine.\n", (long)pid_hijo);

        // El padre espera específicamente al hijo cuyo PID guardó en la variable 'pid_hijo'.
        // El '0' como tercer argumento significa que la llamada es bloqueante.
        /*
        pid (Process ID): Aquí es donde está la magia. Le dices a qué hijo esperar.

        pid > 0: Espera al proceso hijo cuyo ID sea exactamente igual a pid. Este es el uso más común.

        pid == -1: Espera a cualquier proceso hijo. En este caso, se comporta igual que la función wait().

        pid == 0: Espera a cualquier hijo que esté en el mismo grupo de procesos que el padre.

        pid < -1: Espera a cualquier hijo cuyo ID de grupo de procesos sea igual al valor absoluto de pid.*/
        waitpid(pid_hijo, &status, 0);

        // Esta línea solo se ejecutará después de que el hijo haya terminado.
        printf("Padre: Mi hijo ha terminado. ¡Ahora puedo continuar y terminar yo también!\n");
        return 0;
    }

    return 0; // Esta línea nunca se alcanzará
}