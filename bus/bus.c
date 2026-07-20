#include "bus.h"
#include <stdio.h>

// Fijarse de esto si esta bien, creo que no
typedef struct {
  PluginCallback callbacks[PLUGIN_MAX];
  int contadorEvent; // Contador de Eventos de cuantos ingresan al bus = 0;
} eventos;
static eventos lista[EVENTO_CONT];

int bus_init(void) {
  printf("Iniciando bus!\n");
  for (int i = 0; i < EVENTO_CONT; i++) {
    lista[i].contadorEvent = 0;
  }
  return 0;
}

int bus_suscribe(TipoEvento tipo, PluginCallback callback) {
  if (lista[tipo].contadorEvent < PLUGIN_MAX) {
    // lista[tipo] = filas
    lista[tipo].callbacks[lista[tipo].contadorEvent] = callback;
    lista[tipo].contadorEvent = lista[tipo].contadorEvent + 1;
    return 0;
  } else {
    printf("Lista LLENA\n");
    return -1;
  }
}

// bus_publish: Recorre las suscripcioses de UN tipo puntual, el que se esta
// publicando.
void bus_publish(Evento *evento) {
  int fila = evento->tipo;
  // lista[fila].callbacks[i](evento);
  for (int i = 0; i < lista[fila].contadorEvent; i++) {
    lista[fila].callbacks[i](evento);
  }
}

// Bus: Eventos -> Contine 2 Funciones: Revisar/Recibir y Entrega -> Lo que hace
// es traerlo de una LISTA, llamada en este caso LISTA, con el plugins
// =plugin_max Lo que hace es: El bus tiene una lista de plugins que estan
// suscriptos o que el usuario elijio a mi OS integrar, cuando el plugin quiere
// algo, le manda una seÑal al bus y le dice, quiero algo!. El bus lo recibe y
// dice: "¿QUIÉNES están suscriptos a ESTE TIPO de evento?" y se los entrega a
// todos -> busca en la lista de plugins y se fija que plugins es el que se lo
// pidio -> Lo rebisa. Plugin → bus → plugins suscriptos.
