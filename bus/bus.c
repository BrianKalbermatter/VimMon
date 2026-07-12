#include "bus.h"
#include <stdio.h>


// Fijarse de esto si esta bien, creo que no
typedef struct {
  int 20; // este esta en bus.h
}eventos;
static lista [PLUGIN_MAX];

int
bus_init(eventos *evento){
  printf("Iniciando bus!\n");
  // En este caso la lista es el Plugin = *p  
  // SI (lista <> VACIO) ENTONCES
  //   printf("Lista vacia");
  //
  //   leer(*evento);
  //   Grabar(*evento);
  //   SINO
  //     printf("Lista llena"); 
  // FIN_SI
  // 
  // 
}
// Bus: Eventos -> Contine 2 Funciones: Revisar/Recibir y Entrega -> Lo que hace es traerlo de una LISTA, llamada en este caso LISTA, con el plugins =plugin_max 
// Lo que hace es: El bus tiene una lista de plugins que estan suscriptos o que el usuario elijio a mi OS integrar, cuando el plugin quiere algo, le manda una seÑal al bus y le dice, quiero algo!. El bus lo recibe y dice: "¿QUIÉNES están suscriptos a ESTE TIPO de evento?" y se los entrega a todos -> busca en la lista de plugins y se fija que plugins es el que se lo pidio -> Lo rebisa. Plugin → bus → plugins suscriptos.


