// continue: sirve para terminar una iteracion y pasar a la siguiente
// break: no solo termina con la iteracion, sino con el ciclo completo

#include <stdio.h>

int main(){
    int multiplo;
    puts("Escribe el numero que sera ignorado\n");
    scanf("%i", &multiplo);

    for(int i = 0; i < 100; i++){
        if(i == multiplo)
            continue;
        printf("\n%i", i);
    }
        printf("\n\n");
// El ciclo esta definido hasta 99

    for(int i = 0; i < 100; i++){
        if (i==24)
        break;
        printf("\n%i", i);
    }
    return 0;
}
