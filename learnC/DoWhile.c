/*
 * Do - While
 * Sintaxis:
 *  do{
 *      algo (instrucciones)
 *  }while(Condicion)
 *
 */

#include <stdio.h>
int main(){
    int i = 1;

    do{
        printf("%i. \n", i);
        i++;
    }while (i <= 20);
    return 0;
}
