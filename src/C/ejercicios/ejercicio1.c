//Numero Entero a Numeros Romanos
#include <stdio.h>
int nroRomanos();
int switchs();


int main(){
    nroRomanos();
    
    switchs();
    return 0; 
}

// Este calcula los nrosRomanos
int nroRomanos(){
    int numeros, millares, centenas, decenas, unidades;
    printf("Colocar un numero: \n");
    scanf("%i", &numeros);

    unidades= numeros%10; numeros=numeros/10;
    decenas= numeros%10; numeros=numeros/10;
    centenas= numeros%10; numeros=numeros/10;
    millares= numeros%10; numeros=numeros/10;
    
}

// Este contiene los switch
int switchs(int numeros, int millares, int centenas, int decenas, int unidades){
    if (numeros > 100) && (numeros < 999):
        switch(millares){
            case 1: 
            printf("M");
            break;
        case 2: 
            printf("MM");
            break;
        case 3:
            printf("MMM");
            break;
        default:
            break;
        }
    
    }
}
