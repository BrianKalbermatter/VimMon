/*
Cajero automático

  Crear un programa que:
  1. Empiece con un saldo de $1000
  2. Muestre menú: 1) Ver saldo, 2) Depositar, 3) Retirar, 4) Salir
  3. No permita retirar más de lo que hay
  4. Use un do-while para repetir hasta que elija salir
  5. Guardar los saldos en punteros de memoria para que no se pierdan, asi saber cuanto saco la ultima vez!
*/

#include <stdio.h>
#include <stdlib.h>




void initializator();// Prototipo
                     

int saldo;
int main(){
    printf("Colocar el saldo a retirar, porfavor:\n");
    scanf("%i", &saldo);
}
// Todas las funciones que va a tener mi proceso
void initializator(){
    int interactionUser();
}

struct nodo1{
    int dato;// dato: ENTERO;
    struct nodo1 *q; // p*: puntero apunta a nodo1;
};

struct nodo1* agregar_retiro(struct nodo1 *historial, int dato){ // Pregunta porque creo otro registro? y porque ahora el nodo1* lleva un * atras y porque es adelante sino?
    struct nodo1 *nuevo = malloc(sizeof(struct nodo1)); // ahora va adelante! pero de nuevo ahora que seria nuevo?, como tengo que pensar la lista enlazada simple? de un registro de punteros?
                                                        
    nuevo->dato = dato;
    nuevo->q = historial;
    
    printf("Retiro guardado en direccion %p\n", (void*)nuevo); // que significa esta linea? %p muestra el puntero de direccion...pero y (void*)nuevo?
    printf("Valor guardado: %d\n", nuevo->dato);
    return nuevo; // Retorna el nuevo inicio de la lista, ya que se van apilando, enpilada...
}

int interactionUser(){
    // La parte del menu
}


