#ifndef VIMMON_ESCENA_H
#define VIMMON_ESCENA_H

#include "interpreter.h"

// La escena 3D de VimMon. NO es parte del lenguaje PAED.
//
// Esto ya estaba dicho por escrito en `data/escena.json` ("NO es parte del
// lenguaje PAED: es un conjunto de procedimientos que el plugin ide carga
// ADEMAS de sintaxis.json"), pero el codigo no lo cumplia: `CUBO`, `MOVER`,
// `GIRAR` y otros nueve vivian adentro de interpreter.c, y `interp_exec`
// recibia el `SceneState`. Un lenguaje que se instala solo no puede traer
// cubos adentro.
//
// Ahora la escena se REGISTRA desde afuera con `paed_register_proc`, que es la
// misma idea que ya usa el bus de plugins (`bus_register`) y el puerto de
// entrada de LEER (`interp_set_entrada`): el nucleo no conoce a sus
// extensiones, las extensiones se anotan.

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

// Deja la escena vacia y con el fondo en negro.
void escena_init(SceneState *scene);

// Engancha los procedimientos de escena al interprete, apuntando a ESTA
// escena. Hay que llamarlo antes de cada `interp_exec` que deba llenarla: el
// registro guarda el puntero, asi que si la escena cambia de lugar (por
// ejemplo una recarga que arma una escena nueva en la pila), hay que registrar
// de nuevo. Registrar dos veces el mismo nombre reemplaza, no duplica.
void escena_registrar(SceneState *scene);

void escena_print(const SceneState *scene);

#endif // VIMMON_ESCENA_H
