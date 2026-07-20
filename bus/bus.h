#ifndef BUS_H
#define BUS_H
#define PLUGIN_MAX 20

// Los eventos que existen en mi OS
typedef enum { // Para que defino un enumerador aca?
  // EVENTOS:
  // Comparar los strings es lento y el compilador no avisa, solo revienta
  // cuando esta ejecutado.
  EVENTO_TECLA,   // 0
  EVENTO_GUARDAR, // 1
  EVENTO_SALIR,   // 2
  EVENTO_CONT     // centinela
} TipoEvento;

// Un evento: QUE paso + un mensaje, porque quiero mandar un mensaje? y que
// paso?
typedef struct {
  TipoEvento tipo;
  char mensaje[64];
} Evento;

// Puntero a funcion: la direccion de una funcion guardada en una variable

typedef void (*PluginCallback)(Evento *evento);

int bus_init(void);
int bus_suscribe(TipoEvento tipo, PluginCallback callback);
void bus_publish(Evento *evento);
#endif
