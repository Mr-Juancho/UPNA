#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 5

void *PrintHello(void *threadid) {
    
    printf("\n THID: Hola Soy el Hilo %d \n",threadid);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
        pthread_t threads[NUM_THREADS];
        int rc, t;
        int *tid[NUM_THREADS];

        for (t=0; t < NUM_THREADS; t++) {
            tid[t] = (int *)malloc(sizeof(int));
            *tid[t] = t;
            printf("Creando el Hilo %d\n",t);

            rc = pthread_create(&threads[t],NULL,PrintHello, (void *)tid[t]);
	        printf("\n Cree el Hilo THID: %d \n",*(tid[t]));
            
            if (rc) {
              printf("Error, %d\n",rc);
              exit(-1);
	   }
	}
    return 0
}