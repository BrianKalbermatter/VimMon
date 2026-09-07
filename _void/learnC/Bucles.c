/*
 * Bucles o Ciclos
 * La sentencia while
 * Sintaxis:
 *
 * while(algo){
 *      Instrucciones
 * }
 *
 */

#include <stdio.h>

int main(){
    int i = 0; // Comienza con 0
    while(i <= 100){// El error que cometi aca es que coloque el 1 en ves de i, por lo tanto por eso me daba bucle INFINITO
        printf("El valor es %i\n", i);
        i++; // Esto hace que se sume 1 en 1
    }   
}




