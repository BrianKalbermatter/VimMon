#ifndef VIMMON_ENGINE_H
#define VIMMON_ENGINE_H

#include <stdint.h>
#include <stddef.h>   // NULL
#include "../plugins/renderer/renderer.h"

// ============================================================
// VimMon — ENGINE 2D orientado a entidades
//
// "Orientado a objetos" en C = struct de datos + punteros a función.
// Cada Entity lleva sus datos (posición, velocidad, color) y DOS
// callbacks que vos, el que hace el juego, rellenás:
//   - update: qué hace la entidad cada frame (tu lógica).
//   - draw:   cómo se dibuja (opcional; si es NULL, se dibuja un
//             rectángulo con su color).
//
// El motor es dueño del game loop: procesa entrada, llama a tus
// update, limpia la pantalla, dibuja y presenta. Vos solo escribís
// el comportamiento.
// ============================================================

#define ENGINE_MAX_ENTITIES 256

typedef struct Entity Entity;
typedef struct World  World;   // se define completo más abajo

struct Entity {
    float    x, y;       // posición (esquina superior izquierda)
    float    vx, vy;     // velocidad en píxeles por segundo
    int      w, h;       // tamaño en píxeles
    uint32_t color;      // color (usá la macro RGB de renderer.h)
    int      alive;      // 0 = ranura libre / entidad muerta

    void    *state;      // datos propios de tu juego (lo que quieras)

    // Tu lógica. Se llama una vez por frame. Recibe el mundo entero,
    // así podés leer teclas (w->r->key_down), ver otras entidades
    // (entity_overlaps) o consultar el tamaño de pantalla.
    void (*update)(Entity *self, World *w, float dt);
    // Cómo dibujarse. Si es NULL, el motor dibuja un rect con 'color'.
    void (*draw)(Entity *self, const Renderer *r);
};

struct World {
    const Renderer *r;
    Entity   entities[ENGINE_MAX_ENTITIES];
    int      count;              // cuántas ranuras se usaron alguna vez
    uint32_t clear_color;        // color de fondo cada frame
    int      running;            // el loop corre mientras sea != 0
};

// Inicializa el backend (abre ventana) y devuelve el mundo listo.
// Devuelve NULL si algo falló. El puntero es a estado interno estático.
World *engine_init(const Renderer *r, const char *title,
                   int width, int height, uint32_t clear_color);

// Reserva una entidad del pool, ya en cero y viva. NULL si está lleno.
// Configurá sus campos (posición, velocidad, color, update...) sobre
// el puntero devuelto.
Entity *world_spawn(World *w);

// Colisión AABB (caja contra caja). 1 si a y b se solapan.
int entity_overlaps(const Entity *a, const Entity *b);

// Corre el game loop hasta que se cierre la ventana o se presione ESC.
void engine_run(World *w);

// Cierra el backend y libera recursos.
void engine_shutdown(World *w);

#endif // VIMMON_ENGINE_H
