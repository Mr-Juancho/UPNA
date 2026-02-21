#include <stdio.h>
#include <stdlib.h> // Para malloc() y free()
#include <string.h> // Para strlen() y strcpy()

int main() {
    // 1. Un búfer temporal grande y seguro en el Stack.
    char buffer_temporal[1024];

    printf("Por favor, introduce tu nombre completo: ");
    // Leemos la línea del usuario y la guardamos en el búfer temporal.
    fgets(buffer_temporal, sizeof(buffer_temporal), stdin);

    // fgets guarda el "Enter" (\n), vamos a quitarlo para tener la longitud real.
    buffer_temporal[strcspn(buffer_temporal, "\n")] = 0;

    // 2. Medimos la longitud real de lo que el usuario escribió.
    int longitud_real = strlen(buffer_temporal);
    printf("Tu nombre mide %d caracteres.\n", longitud_real);

    // 3. ¡Aquí está la magia! Asignamos la memoria exacta en el Heap.
    // ¡No olvides el +1 para el carácter nulo '\0' que marca el final de una cadena!
    char *nombre_exacto = (char *) malloc(longitud_real + 1);

    // Siempre es buena idea comprobar si malloc pudo darnos la memoria.
    if (nombre_exacto == NULL) {
        fprintf(stderr, "Error: No se pudo asignar memoria.\n");
        return 1;
    }

    // 4. Copiamos el texto del búfer al nuevo espacio de memoria.
    strcpy(nombre_exacto, buffer_temporal);

    printf("Hemos guardado tu nombre '%s' en un bloque de memoria de tamaño perfecto.\n", nombre_exacto);

    // 5. ¡Importantísimo! Cuando ya no necesitemos la memoria, la liberamos.
    free(nombre_exacto);

    printf("Memoria liberada. Fin del programa.\n");

    return 0;
}