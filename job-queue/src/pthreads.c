#include <stdio.h>
#include <pthread.h>

void *saludo_hilo(void *args){ // Esto es un hilo = 1
    printf("Hola mundo desde un hilo\n");
    // Los printf no definen el tipo de la funcion!
    //
    return NULL; // Esto retorna la funcion
}

int main(){ // Esto es un hilo = 1

    pthread_t hilo;
    int resultado;

    // Esta funcion crea el hilo
    resultado = pthread_create(&hilo, NULL, saludo_hilo, NULL);
    
    if(resultado != 0){
        perror("Error al crear el hilo");
        return 1;
    }
    

    // Esta funcion espera a que termine un hilo
    pthread_join(hilo,NULL);

    // gcc pthreads.c -o a.out -lpthread
    return 0;
}
// En total hay 2 Hilos corriendo aca

// En terminos de programacion una estructura 
// selectiva hace referencia a condicionales, por ejemplo, 
// if, else, else if o switch. Para una estructura iteractiva 
// estaremos hablando de ciclos, por ejemplo for, foreach, while odo while.
// Concurrencia, procesos, paralelismo, secuencial
