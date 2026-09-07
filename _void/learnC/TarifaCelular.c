// Calcular tarifas de saldo en celulares y poner precio. "Membrecias"
// De 1000 a 1500 Premium
// De 500 a 999 Intermedia
// De 100 a 499 Basica

#define Tarifa1 "Premium"
#define Tarifa2 "Intermedia"
#define Tarifa3 "Basica"

#include <stdio.h>

int main(){
    float precio;
    char *plan;
    puts("Digite el monto que esta dispuesto a pagar por su plan");
    scanf("%f", &precio);
    //printf("%s", Tarifa1);
    if((precio >= 1000)&&(precio <= 1500))
    {
        puts("El plan que elijio pagar es el Premium");
        plan = Tarifa1;
    }
    else if((precio >= 500)&&(precio <= 999))
    {
        puts("El plan que elijio pagar es la Intermedia");
        plan = Tarifa2;
    }
    else if((precio >= 100)&&(precio <= 499))
    {
        puts("El plan que elijio pagar es el Basico");
        plan = Tarifa3;
    }
    else{
        puts("Elije un plan de pago, porfavor!");
        plan = NULL;
    }

    if(plan != NULL)
    {
        printf("Haz elejido el plan %s\n", plan);
    }else{
        puts("No haz elejido ningun plan!");
    }
    return 0;
}
