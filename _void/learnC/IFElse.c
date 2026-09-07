#include <stdio.h>
int main(){
    float a, b, c;
    
    printf("Digite el numero a\n");
    scanf("%f", &a);
    printf("Digite el numero b\n");
    scanf("%f", &b);
    c = a + b;
    if(c >= 200){
        printf("Primera opcion!");
    }if(c <= 200){
        printf("Segunda opcion!");
    }if(c == 200){
        printf("Tercera opcion");
    }
    return 0; 
}
