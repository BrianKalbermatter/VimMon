#include <stdio.h>

int main()
{
    int numeros;
    int millar;
    int centenas;
    int decenas;
    int unidades;
    printf("Ingresa un numero: \n");
    scanf("%d", &numeros);

    unidades = numeros%10; numeros = numeros/10;
    decenas = numeros/100; numeros = numeros/10;
    centenas = numeros/100; numeros = numeros/100;
    millar = numeros/100; numeros = numeros/1000;

    switch (millar)
    {
        case 1:
            printf("M"); // 1mil
            break;
        case 2:
            printf("MM"); // 2mil
            break;
        case 3:
            printf("MMM"); // 3mil
            break;
    }
    switch (centenas)
    {
        case 1:
            printf("C"); // 100
            break;
        case 2:
            printf("CC"); // 200
            break;
        case 3:
            printf("CCC"); // 300
            break;
        case 4:
            printf("CD"); // 400
            break;
        case 5:
            printf("D"); // 500
            break;
        case 6:
            printf("DC"); // 600
            break;
        case 7:
            printf("DCC"); // 700
            break;
        case 8:
            printf("DCCC"); // 800
            break;
        case 9:
            printf("CM"); // 900
            break;
    }

    switch (decenas)
    {
        case 1:
            printf("X");
            break;
        case 2:
            printf("XX");
            break;
        case 3:
            printf("XXX");
            break;
        case 4:
            printf("XL");
            break;
        case 5:
            printf("L");
            break;
        case 6:
        case 7:
        case 8:
        case 9:


    }
    switch (unidades)
    {
        case 1:
            printf("I");
            break;
        case 2:
            printf("II");
            break;
        case 3:
            printf("III");
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
    }
    return 0;
}

