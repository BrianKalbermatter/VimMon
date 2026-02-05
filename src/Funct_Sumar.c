#include <stdio.h>

int Sumar(int n1, int n2); // En el prototipo debe coincidir con la definicion de la funcion original
int Suma = 0, a ,b;

int my_function_Sumar_main(){
    printf("Escribe 2 numeros");
    scanf("%i %i",&a, &b);

    Suma = Sumar(a, b);
    printf("La suma es %i", Suma);

    return 0;
}

int Sumar(int n1, int n2){ // Funcion original
    int suma = 0;
    suma = n1+n2;
    return suma;
}
// En C, cuando pasas un parametro a una funcion, se pasa por valor. Esto significa que la funcion recibe una copia del valor, no la variable original.

