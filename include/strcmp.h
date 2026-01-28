#include <stdio.h>
#include <string.h>

// Que pasa si no coloco el include guarde... Puede que se ejecute varias veces dentro del main el .header
#pragma once

void* espera_ingresar_nuevamente(void* arg){
    while
    return NULL;
}

int strcmp_h(){
    /* strcmp (cadena1, cadena2)
     * Compara 2 cadenas de texto caracter a caracter, es case-sensitive(sensible a mayusculas y minusculas). Cuando se encuentra una diferencia, esta funcion devuelve un valor entero correspondiente a la diferencia de valor decimal segun el codigo ASCII. El cual corresponde a la siguiente tabla.
     *
     * Si la cadena1 es        Entonces devuelve 
     * Igual que cadena2         0
     * Mayor que cadena2       n > 0
     * Menor que cadena2       n < 0
     * */
    char clave_secreta[] = "Seguro";
    char usuario_digito[128];
    int intento_restantes = 3;
    
    do{
        printf("\n\n Escribe la clave secreta: ");
        fflush(stdin);
        // Lo que hace esta funcion es intenta limpiar el buffer de entrada, pero tecnicamente 
        // es comportamiento indefinido en C estandar
        // Evitar este tipo de codigo portable, causa problemas...
        scanf("%s", usuario_digito); // el & sobra aca porque ya es un arreglo, asi que ya es un puntero.
        // Validacion de Clave

        if(strcmp(clave_secreta, usuario_digito)==0){
        printf("\n Bienvenido al sistema");
        break;
        }if else{
            intento_restantes--;
            printf("Clave secreta incorrecta, le quedan: %i intentos:", intento_restantes);
            
        }else{
            puts("Haz quedado bloqueado!, espera 30 min. Luego vuelve a intentarlo");
            espera_ingresar_nuevamente();
        }
    }while(intento_restantes > 0);

    return 0;
}
