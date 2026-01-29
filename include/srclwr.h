// strlwr(cadena)
// Esta funcion de string.h lo que hace es que convierte a minusculas todas las letras de una cadena de texto
#include <stdio.h>
#include <string.h>

int strlwr_h()
{
    char texto[100];
    puts("Escribe un texto con mayusculas y minusculas\n");
    fflush(stdin);
    scanf("%s", texto);
    
    puts("El nuevo texto es el siguiente:\n");
    printf("%s\n", strlwr(text));
    return 0;
}
