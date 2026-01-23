// Preincremento y postincremento
// 
// - Preincremento
// int a = 0;
// int b = ++a;
// 
// Output:
//      a=1 y b=1.
// 
// - PostIncremento
// int a = 0;
// int b = a++;
//
// Output:
//      a = 1 y b = 1, esto significa que b obtuvo el valor de a anterior al incremento
// En los bucles for se puede usar el que queramos conveniente...
int main(){
    int a = 0;
    int b = ++a;

    printf("El valor de a es de: %i\n", a);
    printf("El valor de b es de: %i\n", b);
    printf("\n\n");

    int c = 0;
    int d = c++;

    printf("El valor de c es de: %i\n", c);
    printf("El valor de d es de: %i\n", d);

    return 0;
}
