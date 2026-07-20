#include "plugin.h"
#include <stdio.h>

// El bus tiene una lista de plugins suscriptos a cada tipo de evento.
// Cuando alguien publica (bus_send) un evento, el bus busca quienes estan
// suscriptos a ESE tipo y les entrega el evento via su on_event().
// Plugin -> bus -> plugins suscriptos.

typedef struct {
  Plugin *plugins[PLUGIN_MAX];
  int cantidad;
} suscriptores;

static suscriptores lista[EVENT_COUNT];

int bus_init(void) {
  printf("Iniciando bus!\n");
  for (int i = 0; i < EVENT_COUNT; i++) {
    lista[i].cantidad = 0;
  }
  return 0;
}

int bus_register(Plugin *plugin, EventType *inputs, int input_count) {
  for (int i = 0; i < input_count; i++) {
    EventType tipo = inputs[i];

    if (lista[tipo].cantidad >= PLUGIN_MAX) {
      printf("bus_register: lista llena para el tipo %d\n", tipo);
      return -1;
    }

    lista[tipo].plugins[lista[tipo].cantidad] = plugin;
    lista[tipo].cantidad++;
  }

  return 0;
}

void bus_send(EventType type, void *data, uint32_t size) {
  Event evento = { .type = type, .data = data, .size = size };

  for (int i = 0; i < lista[type].cantidad; i++) {
    lista[type].plugins[i]->on_event(&evento);
  }
}

void bus_unregister(Plugin *plugin) {
  for (int t = 0; t < EVENT_COUNT; t++) {
    for (int i = 0; i < lista[t].cantidad; i++) {
      if (lista[t].plugins[i] == plugin) {
        lista[t].plugins[i] = lista[t].plugins[lista[t].cantidad - 1];
        lista[t].cantidad--;
        i--;
      }
    }
  }
}

void bus_shutdown(void) {
  Plugin *ya_apagados[PLUGIN_MAX];
  int cantidad_apagados = 0;

  for (int t = 0; t < EVENT_COUNT; t++) {
    for (int i = 0; i < lista[t].cantidad; i++) {
      Plugin *p = lista[t].plugins[i];

      int ya_esta = 0;
      for (int j = 0; j < cantidad_apagados; j++) {
        if (ya_apagados[j] == p) {
          ya_esta = 1;
          break;
        }
      }

      if (!ya_esta) {
        p->shutdown();
        ya_apagados[cantidad_apagados] = p;
        cantidad_apagados++;
      }
    }
  }

  for (int t = 0; t < EVENT_COUNT; t++) {
    lista[t].cantidad = 0;
  }

  printf("Bus apagado.\n");
}
