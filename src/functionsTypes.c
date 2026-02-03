//Funciones
//Funciones sin retorno de valor:
void nombreFuncion(char *str){
    //Instruccions
}

//Funciones con retorno de valor:
char nombre(char *str){
    //Instrucciones
    return *str; // Expresion
}

// Escribir holaMundo con uso de FUNCIONES!
// Sumar dos numeros:

#include <stdio.h>
#include <string.h>

#include "strupr.h"
#include "strlwr.h"
// La tarea la hare mas complicada, voy a crear un condicional en el que puedas elegir entre si o no con minusculas y si le pasas en mayus, no va a importar... Llamara a las funciones que ya tengo creada...

void conditionaldeFuntions();


int suma(int num1, int num2){
    int resultado;
    puts("Podrias decirme los numeros que quieres sumar?\n");
    scanf("%i", &num1);
    printf("Numero 1: %i", num1);
    scanf("%i", &num2);
    printf("Numero 2: %i", num2);
    resultado = num1 + num2;


    return resultado;
}


void conditionaldeFuntions(char *str){
    char option[10] = " ";
    puts("Dime una opcion: SI o NO? \n");
    char fgets(char *option, int n, FILE *option);
    if (){

    }
}


