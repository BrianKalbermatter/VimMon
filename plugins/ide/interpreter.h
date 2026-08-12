#ifndef VIMMON_INTERPRETER_H
#define VIMMON_INTERPRETER_H

#include <stddef.h>   // size_t

#include "parser.h"

#define SCENE_MAX_CUERPOS 64

typedef struct { float x, y, z; } Vec3;

typedef struct {
    char  id[PAED_NAME_MAX];
    char  kind[PAED_NAME_MAX];   // cubo | esfera | plano | luz
    char  grupo[PAED_NAME_MAX];  // "" = suelto. Varias piezas con el mismo
                                 // grupo se mueven, rotan y escalan juntas:
                                 // asi una "nave" es 8 cubos y UNA sola cosa.
    Vec3  position;
    Vec3  rotation;
    Vec3  scale;
    char  color[16];

    float radio;                 // esfera
    char  luz_tipo[16];          // luz: puntual | dir
    float intensidad;            // luz

    char  giro_eje;              // comportamiento GIRAR: 'x' | 'y' | 'z' | 0
    float giro_velocidad;
    float osc_amplitud;          // comportamiento OSCILAR
    float osc_frecuencia;
} Cuerpo;

typedef struct {
    Cuerpo cuerpos[SCENE_MAX_CUERPOS];
    int    cuerpo_count;
    char   bg_color[16];
    Vec3   cam_pos;
    Vec3   cam_target;
} SceneState;

// ── De donde salen los datos de LEER ─────────────────────────────────────────
//
// El interprete NO abre stdin por su cuenta, y no es un capricho: corre DENTRO
// del game loop del renderer, asi que un fgets bloqueante congelaria la ventana
// entera esperando que alguien tipee en una terminal que quiza ni esta a la
// vista. El que hospeda al interprete es el que sabe de donde vienen los datos:
// paedrun engancha stdin, la ventana SDL todavia no engancha nada.
//
// Deja UNA linea en `buf`, sin el '\n'. Devuelve 0 si trajo un dato, -1 si la
// entrada se termino.
typedef int (*PaedEntrada)(char *buf, size_t n, void *ud);

// Engancha la fuente de datos. Sin fuente, LEER de consola falla con un mensaje
// claro en vez de colgarse esperando algo que nunca va a llegar.
void interp_set_entrada(PaedEntrada fn, void *ud);

void interp_init (SceneState *scene);

// Ejecuta el programa. Devuelve 0 si se ejecuto entero.
// Los errores en runtime se reportan con archivo:linea, igual que el parser.
int  interp_exec (SceneState *scene, const PAEDProgram *prog);

void interp_print(const SceneState *scene);

#endif // VIMMON_INTERPRETER_H
