#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#define MAX_THREAD 10

 void *hace_algo(void *para1)
 {
 int i, suma;
 int salida = 0;
    
 printf("El argumento que recibi es %d\n",para1);

 for (i=0;i<10;i++)
    salida = salida + i;
    
    pthread_exit((void *)salida);
 }

 main(int argc, char *argv[])
 {
      pthread_t tid[MAX_THREAD];
      int i, status;
      int salida, creados=0;
 
      for (i=0; i < MAX_THREAD; i++) {
        if (pthread_create(&tid[i],NULL,hace_algo, (void *)i))
             fprintf(stderr,"No se pudo crear %d: %s \n",i,strerror(errno));
        else
           fprintf(stderr,"Se creo el hilo %u \n",tid[i]);
 }
   for (i=0; i < MAX_THREAD;i++) {
      if (pthread_join(tid[i],(void *)&salida)!=0)
          fprintf(stderr, "No hay hilo %d: %s \n",i,strerror(errno));
      else
          fprintf(stderr, "Termino el hilo %u con salida %d\n",tid[i],salida);
     }
 } 



