// Menor de dos Numeros
#include <stdio.h>

int main(){
    int num1, num2;
    puts("Digite dos numeros para saber cual es el menor de los dos!");
    scanf("%i %i",&num1,&num2);


    if (num1 < num2){
        printf("El meyor es %i", num1);
    }else{
        printf("El menor es %i", num2);
    }
}
