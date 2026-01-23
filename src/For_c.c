#include <stdio.h>

void main(){
    // Declarar
    int i, n;
    printf("Ingresa la cantidad de numeros pares!.\n");
    scanf("%d", &n);
    for(i = 1; i <= n; i++){
        printf("i: %d\n", i * 2);
    }
}
