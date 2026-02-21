#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

struct mensajeAlerta {
    long tipo;
    char texto[100];
};

int tomaMedida() {
 return rand() % 100;
} 

// ./anemometro clave periodoAnem umbralAnem
int main(int argc, char *argv[]) {

    if (argc < 4){
        fprintf(stderr, "Numero de argumentos invalido\n");
        exit(EXIT_FAILURE);
    }

    char clave[100];
    strcpy(clave, argv[1]);
    int periodoAnem = atoi(argv[2]);   
    int umbralAnem = atoi(argv[3]);   

    //misma clave que usa meteoestacion
    key_t clave_memoria = ftok(clave, 'A');
    if (clave_memoria == -1) {
        perror("surtidor: ftok");
        exit(EXIT_FAILURE);
    }
    int id_cola = msgget(clave_memoria, 0666);
    if (id_cola == -1){
        perror("Error al crear la cola de mensajes");
        exit(EXIT_FAILURE);
    }

    
    struct mensajeAlerta msg;
    while (1) {
        sleep(periodoAnem);


        int vientoMedida = tomaMedida();
        printf("La medida del viento es %d\n",vientoMedida);
        if (vientoMedida > umbralAnem){
            msg.tipo = 1;
            snprintf(msg.texto,sizeof(msg.texto),"Alerta viento fuerte de %d metros por segundo",vientoMedida);
            if(msgsnd(id_cola, &msg, sizeof(msg),0) == -1){
                perror("Error anemometro msg snd");
                exit(EXIT_FAILURE);
            }
        }

    }
    return 0;
}
