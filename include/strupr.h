// strupr(Cadena)
// Contrario a strlwr, esta funcion convierte a mayusculas las letras de una cadena
//
#include <stdio.h>
#include <string.h>
#include <ctype.h>
// Problema:
//  Lo que hice aca es: Como tenia un problema strupr() no es estandar propiamente de linux porque
//  printf("%s\n", strupr(text));  // ❌ Error en C99/GCC
//  - strupr() convierte texto a mayúsculas
//  - Solo existe en compiladores viejos (Turbo C, Borland) y Windows (MSVC)
//  - GCC en Linux no la tiene → error de compilación
//  Solucion:
//  Crear mi propia funcion usando toupper() que SI es estandar:
char *my_strupr(char *str){
    char *p = str;
    while (*p){
        *p = toupper(*p);
        p++;
    }
    return str;
}
int strupr_h(){
    char text[100] = " ";
    puts("Escribe un texto con mayusculas y minusculas\n");
    
    // fflush(stdin);
    // En windows esta funcion fflush si funciona por condiciones de windows. Tiene sus extensiones pero en linux hace cosas raras por lo tanto se cambia a una iteacion con un bucle EOF
    int c;
    while((c = getchar())!= '\n' && c != EOF);



    fgets(text, sizeof(text), stdin);

    printf("El nuevo texto es el siguiente:\n");
    printf("%s\n", my_strupr(text));
    return 0;  
}
