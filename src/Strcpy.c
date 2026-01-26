// strcpy (Destino, Fuente)
// Permite copiar una cadena de texto en otra. Debes considerar que la dimension del array
// Destino debe ser igual o mayor que el array origin y que ademas sobrescribe el contenido destino(de existir).

#include <stdio.h>
#include <string.h>

void miFuncion(){
    char caracteres[] = "Hola como estas?";
    char caracteresOs[sizeof(caracteres)];
    int ventanaDeCaracteres = 0; // Este para el indice de recorrido, tiene que comenzar en 0
    strcpy(caracteresOs, caracteres);
    while(caracteresOs[ventanaDeCaracteres] != '\0') // El \0 es el Null en Strings como la Secuencia de Caracteres en AED! 
    {
        printf("%c", caracteresOs[ventanaDeCaracteres]);
        ventanaDeCaracteres ++;
    }
}
void initializate(){
    miFuncion();
}
int main(){

    initializate();
    char origen[]="Programacion"; // Aunque no tenga definido el numero de indice de la cantidad de caracteres se define luego automaticamente
    // Dado que programacion tiene 12 caracteres.
    // Creamos el array destino de la misma dimension
    
    char destino[sizeof(origen)]; // sizeof es un operador en C, no es una funcion. Devuelve el tamano en bytes de una variable o tipo de dato
    strcpy(destino, origen);
    printf("%s", destino);
    return 0;
}
