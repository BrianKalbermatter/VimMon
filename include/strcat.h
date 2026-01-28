#ifndef Strcat_h 
#define Strcat_h
#include <stdio.h>
#include <string.h>
// strcat tiene un(Destino, Fuente)
// Concatena (Agrega) la cadena fuente en el destino. Debes considerar que la cadena destino debe tener un tamano tal que pueda albergar la cadena resultante.

int strcat_h(){
    char cadena1[] = "Brian", cadena2[] = "Ricardo";
    char final[50] = "";
    //Concatenamos
    strcat(final,cadena1);
    strcat(final, "-");
    strcat(final, cadena2);
    printf("%s", final);
    return 0;
}
#endif


