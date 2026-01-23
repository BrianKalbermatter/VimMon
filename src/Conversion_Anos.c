#include <stdio.h>
int main(){
    int edad, meses, semanas, dias, horas;
    printf("Digita tu edad\n");
    scanf("%i", &edad);
    
    meses = edad * 12;
    semanas = meses * 4;
    dias = semanas * 7;
    horas = dias * 24;

    printf("Tu edad en Meses es de %i\n", meses);
    printf("Tu edad en Semanas es de %i\n", semanas);
    printf("Tu edad en Dias es de %i\n", dias);
    printf("Tu edad en horas es de %i\n", horas);
    printf("Tu edad todo completo es: %i %i %i %i", meses, semanas, dias, horas);
    return 0;
}
