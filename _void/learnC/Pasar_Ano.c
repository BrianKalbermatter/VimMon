// Ver si un alumno va a pasar de ano o no. Un alumno tiene derecho a reprobar 3 materias para poder pasar de ano, si reprueba 4 materias no puede pasar de ano y recursa.

#include <stdio.h>

int main(){
    int materias;
    puts("Cuantas materias reprobaste?");
    scanf("%i", &materias);
    if(materias <= 3){
        printf("Estas bien, no recursaras!");
    }else{
        printf("Estudia mas!");
    }
    return 0;
}

