#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> // Para malloc, free, exit
#include <string.h> // Para strlen

int main() {
    int tuberia[2];
    pid_t pid;

    if (pipe(tuberia) == -1) {
        perror("pipe falló");
        return 1;
    }

    pid = fork();

    if (pid == -1) {
        perror("fork falló");
        return 1;
    }

    if (pid == 0) { // Proceso HIJO (escritor)
        char *cadena = "Hola mundo";
        close(tuberia[0]); // Cierra el extremo de lectura

        // CORRECCIÓN: Llamada a write con 3 argumentos
        write(tuberia[1], cadena, strlen(cadena) + 1);

        close(tuberia[1]); // Cierra el extremo de escritura
        exit(0); // Usa 0 para salida exitosa
    } else { // Proceso PADRE (lector)
        char *cadena;
        int bytesLeidos;

        // CORRECCIÓN: Llamada a malloc
        cadena = (char *) malloc(100);
        if (cadena == NULL) {
            perror("malloc falló");
            return 1;
        }

        close(tuberia[1]); // Cierra el extremo de escritura

        // CORRECCIÓN: Llamada a read con 3 argumentos
        bytesLeidos = read(tuberia[0], cadena, 100);

        // Buena práctica: asegurar el terminador nulo
        if (bytesLeidos > 0) {
            cadena[bytesLeidos] = '\0';
        }
        printf("Mensaje leído: %s\n", cadena);
        close(tuberia[0]); // Cierra el extremo de lectura

        // MEJORA: Liberar la memoria para evitar fugas
        free(cadena);
        exit(0); 
    }
    // Esta parte del código nunca se alcanza
    return 0;
}