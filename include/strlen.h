// strlen(Cadena)
// Devuelve un entero que representa la longitud de una cadena de texto (incluyendo espacios en blanco, pero expluyendo el caracter nulo). \0 hola "h","o","l","a","\0"

#ifndef STRLEN_H 
#define STRLEN_H
#include <stdio.h>
#include <string.h>

int strlen_h(){
    // La dimension del array es 200, pero la cadena puede ser menor
    char array[200];
    printf("Escribe cualquier cosa\n");
    fflush(stdin);
    scanf("%s", &array);
    printf("\n La longitud de la cadena digitada es: %i", strlen(array));
    return 0;
}
#endif
