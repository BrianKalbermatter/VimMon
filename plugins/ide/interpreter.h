#ifndef VIMMON_INTERPRETER_H
#define VIMMON_INTERPRETER_H

#include "parser.h"

#define SCENE_MAX_ENTITIES 64

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
} Entity;

typedef struct {
    Entity entities[SCENE_MAX_ENTITIES];
    int    entity_count;
    char   bg_color[16];
    Vec3   cam_pos;
    Vec3   cam_target;
} SceneState;

void interp_init (SceneState *scene);

// Ejecuta el programa. Devuelve 0 si se ejecuto entero.
// Los errores en runtime se reportan con archivo:linea, igual que el parser.
int  interp_exec (SceneState *scene, const PAEDProgram *prog);

void interp_print(const SceneState *scene);

#endif // VIMMON_INTERPRETER_H
