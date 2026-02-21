#include <stdio.h>      // Para printf, fopen, fgets (Entrada/Salida)
#include <stdlib.h>     // Para exit, malloc
#include <unistd.h>     // Para fork, execvp, alarm, pause
#include <string.h>     // Para manipular texto (strtok)
#include <sys/types.h>  // Definiciones de tipos de datos del sistema
#include <sys/wait.h>   // Para esperar procesos (wait)
#include <sys/ipc.h>    // Para IPC (comunicación entre procesos)
#include <sys/msg.h>    // Para colas de mensajes
#include <errno.h>      // Para ver errores
#include <signal.h>

struct mensaje{
    long mtype;
    pid_t pid;
    int level;
    int priority;
};

// Variable global para que la función de limpieza la vea
int idcola_global; 
int alarma_activada = 0; // Bandera para saber si saltó el tiempo

// --- ESTADISTICAS ---
int stats_rr = 0;          // Finalizados en Nivel 1
int stats_prio = 0;        // Finalizados en Nivel 2
int stats_fcfs = 0;        // Finalizados en Nivel 3
int stats_contexto = 0;    // Cambios de contexto (veces que damos la CPU)

void manejar_salida(int senal) {
    printf("\nLimpiando recursos...\n");
    msgctl(idcola_global, IPC_RMID, NULL); // Borrar la cola

    // Imprimir estadísticas finales
    printf("\n=== ESTADISTICAS ===\n");
    printf("Procesos finalizados (Round Robin): %d\n", stats_rr);
    printf("Procesos finalizados (Prioridad):   %d\n", stats_prio);
    printf("Procesos finalizados (FCFS):        %d\n", stats_fcfs);
    printf("------------------------------------\n");
    printf("TOTAL procesos finalizados:         %d\n", stats_rr + stats_prio + stats_fcfs);
    printf("TOTAL cambios de contexto:          %d\n", stats_contexto);
    printf("====================\n");


    kill(0, SIGKILL); // Opcional: Matar a todos los hijos pendientes   
    exit(0);
}

void manejar_alarma(int s) {
    alarma_activada = 1;
}
void manejador_hijo(int s) {
    // No hace nada, solo interrumpe el pause()
}


int main(int argc, char *argv[]){


    FILE *entrada;
    //Abrir archivo de entrada
    if(argc >1){
        entrada = fopen(argv[1],"r");
        if (entrada == NULL){
            fprintf(stderr,"Error al abrir el archivo");
            exit(1);
        }
    }else{
        entrada = stdin;
    }
    // Crear cola de mensajes
    key_t clave = ftok("procsched.c",65);
    int idcola = msgget(clave, IPC_CREAT | 0666);
    if(idcola == -1){
        perror("Error al crear la cola");
        exit(1);
    }

    idcola_global = idcola;
    signal(SIGINT, manejar_salida);
    signal(SIGALRM, manejar_alarma);
    signal(SIGCHLD, manejador_hijo);

    //Leer archivo de entrada
    char linea[1024];
    char *token;
    char *args[100];

    //nivel prioridad nombreprograma argumento1 argumento2 ...
    while(fgets(linea, sizeof(linea), entrada) != NULL){
        struct mensaje msg;

        token = strtok(linea, " \n\t\r"); //leemos el nivel
        if(token == NULL) continue; // Protección contra líneas vacías
        msg.level = atoi(token);
        msg.mtype = msg.level;

        token = strtok(NULL, " \n\t\r"); //leemos la prioridad
        msg.priority = atoi(token);

        token = strtok(NULL, " \n\t\r"); //leemos el nombre del programa
        args[0] = token;
        int i = 1;
        while(token != NULL){
            token = strtok(NULL, " \n\t\r");
            args[i] = token;
            i++;
        }
        args[i] = NULL;
        msg.pid = fork();
        if(msg.pid == -1){
            perror("Error al crear el proceso");
            exit(1);
        }
        if(msg.pid == 0){
            raise(SIGSTOP);
            execvp(args[0], args);
            perror("Error al ejecutar el programa");
            exit(1);
        }
        if(msgsnd(idcola, &msg, sizeof(msg)-sizeof(long), 0) == -1){
            perror("Error al enviar el mensaje");
            exit(1);
        }    
    }

    fclose(entrada); //ya no necesitamos el archivo

    
    printf("Inicio del planificador\n");
    struct mensaje msg;
    while(1){
        
        if(msgrcv(idcola,&msg, sizeof(msg)-sizeof(long),1,IPC_NOWAIT) != -1){
            printf("[Nivel 1] Round robin: ejecutando PID %d\n", msg.pid);

            alarma_activada = 0;
            kill(msg.pid, SIGCONT); // Reanudamos
            alarm(4);               // Ponemos cronómetro

            int proceso_finalizado = 0;

            while (!alarma_activada && !proceso_finalizado) {
                pause(); // Esperamos CUALQUIER señal

                int pid_revisado = waitpid(msg.pid, NULL, WNOHANG);
                
                if (pid_revisado == msg.pid) {
                    proceso_finalizado = 1; // ¡Murió de verdad!
                }
            }
            
            alarm(0); // Apagamos alarma por si acaso

            if (proceso_finalizado) {
                // Caso A: Terminó su trabajo
                printf("Proceso %d terminado (antes de 4s)\n", msg.pid);
                stats_rr++;
            } else {
                // Caso B: Saltó la alarma (se acabó el tiempo)
                printf("[Nivel 1] Fin de turno PID %d. Pausando...\n", msg.pid);
                kill(msg.pid, SIGSTOP);
                msgsnd(idcola, &msg, sizeof(msg) - sizeof(long), 0); // Al final de la cola
            }
            continue;
        }
        if(msgrcv(idcola, &msg, sizeof(msg) - sizeof(long), 2, IPC_NOWAIT) != -1){
            struct mensaje lista_pendientes[100];
            int count = 0;

            lista_pendientes[count++] = msg; // Guardamos el primero que sacamos

            // Sacamos el resto
            while(msgrcv(idcola, &msg, sizeof(msg) - sizeof(long), 2, IPC_NOWAIT) != -1){
                lista_pendientes[count++] = msg;
            }

            // 2. Buscar el de mejor prioridad (menor valor)
            int idx_mejor = 0;
            for(int i = 1; i < count; i++){
                if(lista_pendientes[i].priority < lista_pendientes[idx_mejor].priority){
                    idx_mejor = i;
                }
            }

            // 3. Ejecutar el mejor
            struct mensaje mejor_msg = lista_pendientes[idx_mejor];
            printf("[Nivel 2] Prioridad %d: Ejecutando PID %d\n", mejor_msg.priority, mejor_msg.pid);

            stats_contexto++;
            kill(mejor_msg.pid, SIGCONT);
            int status;
            waitpid(mejor_msg.pid, &status, 0); // Esperamos a que termine

            printf("Proceso %d terminado\n", mejor_msg.pid);
            stats_prio++;

            // 4. Devolver los NO elegidos a la cola
            for(int i = 0; i < count; i++){
                if(i != idx_mejor){
                    msgsnd(idcola, &lista_pendientes[i], sizeof(msg) - sizeof(long), 0);
                }
            }
            
            continue;
        }

        if(msgrcv(idcola, &msg, sizeof(msg) -sizeof(long), 3, IPC_NOWAIT) != -1){
            printf("Ejecutando proceso %d con prioridad %d\n", msg.pid, msg.priority);

            stats_contexto++;
            kill(msg.pid, SIGCONT); //despertamos el proceso

            int status;
            waitpid(msg.pid, &status, 0); //esperamos a que el proceso termine (NO APROPIATIVO)
            printf("Proceso %d terminado\n", msg.pid);
            
            stats_fcfs++;
            continue;
        }
        usleep(10000);
    }
    return 0;
}