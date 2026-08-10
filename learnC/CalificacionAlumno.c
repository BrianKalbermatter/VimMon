// Decirle al alumno algo con base en sus calificaciones
// 9 - 10 -> Excelente, sigue asi
// 8 - 9 -> Muy bien, puedes mejorar
// 7 - 8 -> Eres un estudiante regular
// 0 - 6 -> Puedes mejorar, tal vez no!

#include <stdio.h>
int main(){
    int calif;
    puts("Digita tu Calificacion\n");
    scanf("%i", &calif);
    switch(calif){
        case 1:
            printf("Te sacaste un 1!!!. Vas mal. Estudia mas!");
            break;
        case 2:
            printf("Te sacaste un 2!!!. Vas mal. Estudia mas!");
            break;
        case 3:
            printf("Te sacaste un 3!!!. Vas mal. Estudia mas!");
            break;
        case 4:
            printf("Te sacaste un 4!!!. Vas mal. Estudia mas!");
            break;
        case 5:
            printf("Te sacaste un 5!!!. Vas mal. Estudia mas!");
            break;
        case 6:
            printf("Te sacaste un 6!!!. Aprobaste. Puedes seguir mejorando!");
            break;
        case 7:
            printf("Te sacaste un 7!!!. Aprobaste. Puedes seguir mejorando");
            break;
        case 8:
            printf("Te sacaste un 8!!!. Super!");
            break;
        case 9:
            printf("Te sacaste un 9!!!. Super Bien!!!");
            break;
        case 10:
            printf("Ja la tenes clarisima!!!");
            break;
        default:
            printf("No es una nota valida, coloca una que entre en el rango del 1 al 10 porfas!");
            break;
    }
    return 0; // Esto devuelve al sistema operativo el valor 0 que significa que todo funciona bien.
}

