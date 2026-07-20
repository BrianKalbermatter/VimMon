#ifndef VIMMON_PLUGIN_H
#define VIMMON_PLUGIN_H

#include <stdint.h>

#define PLUGIN_MAX 20 // maximo de plugins suscriptos por tipo de evento

// ============================================================
// VimMon — PLUGIN CONTRACT v1.0
// Todos los plugins implementan esta interfaz.
// ============================================================

// Los tipos de eventos que viajan por el bus
typedef enum {
    EVENT_NONE = 0,
    EVENT_KEYBOARD,        // tecla presionada/liberada
    EVENT_MOUSE,           // click o movimiento de mouse
    EVENT_AI_REQUEST,      // prompt enviado a la IA
    EVENT_AI_RESPONSE,     // respuesta de la IA (PAED delta)
    EVENT_SCENE_UPDATE,    // escena cambió (nuevo delta en scene.paed)
    EVENT_RENDER_FRAME,    // es momento de renderizar
    EVENT_MONITOR_TICK,    // actualización de stats del sistema
    EVENT_SHUTDOWN,        // el OS se está apagando
    EVENT_COUNT,           // centinela: cantidad total de tipos de evento
} EventType; //typedef

// Contenedor genérico de evento
typedef struct {
    EventType  type;
    void      *data;
    uint32_t   size;
} Event;

// ============================================================
// INTERFAZ 
// ============================================================
typedef struct {
    const char *name;
    const char *version;

    int  (*init)(void);          // carga el plugin, retorna 0 si OK
    void (*shutdown)(void);      // limpieza al apagar
    void (*tick)(float delta);   // se llama cada frame
    void (*on_event)(Event *e);  // recibe eventos del bus
} Plugin;

// ============================================================
// BUS API — lo que los plugins pueden llamar
// ============================================================
int  bus_init(void);
void bus_send(EventType type, void *data, uint32_t size);
int  bus_register(Plugin *plugin, EventType *inputs, int input_count);
void bus_unregister(Plugin *plugin);
void bus_shutdown(void);

#endif // VIMMON_PLUGIN_H
