// Determinar suma de todos los numeros hasta el numero que de el usuario

#include <stdio.h>

int main(){
    int cont, num, suma = 0; // Si aca no incicializaba a 0, suma entonces iba a tener un valor "basura".
    puts("Escribe hasta que el numero quieres que se haga la suma: \n");
    scanf("%i", &num);
    
    cont = 1;

    while(cont<=num){
        suma += cont;// Equivale a: suma = suma + cont .
        cont++;

    }
    printf("La suma hasta el numero %i es de %i", num, suma);

    return 0;
}
