#include <stdio.h>

int main(){
    int horas;
    int salario_horas;
    int salario_total;
    
    printf("Digite las horas trabajadas\n");
    scanf("%d",&horas);

    printf("Digite el salario de horas trabajadas\n");
    scanf("%d", &salario_horas);
    
    salario_total = horas * salario_horas;

    printf("El salario total de horas trabajadas es: $ %d", salario_total);
    return 0;
}
