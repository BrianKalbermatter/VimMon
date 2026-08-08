#ifndef VIMMON_SCENE_VIEW_H
#define VIMMON_SCENE_VIEW_H

#include "../../engine/engine.h"

// Puente entre la escena PAED (3D, declarativa) y el motor 2D (pixeles).
//
// El motor no sabe nada de PAED ni PAED sabe nada del motor: los une UNA sola
// entidad del pool, cuyo update vigila el archivo y cuyo draw proyecta la
// escena. Por eso engine.c no necesito tocarlo ni una linea — el contrato de
// Entity (update + draw) ya alcanzaba.
//
// Recarga en caliente: cada media segundo se mira el mtime de paed_path. Si
// cambio, se re-parsea y se redibuja sola. Es el mismo mecanismo que usa
// hot-Reload/host.c para game.so, pero vigilando un .paed: aca no hay codigo
// que recargar, solo una descripcion de escena.
//
// Si el archivo nuevo tiene errores, se CONSERVA la ultima escena buena y se
// reportan por consola. Quedarte con la pantalla en negro porque te comiste un
// ';' seria el peor comportamiento posible mientras estas editando.
int scene_view_mount(World *w, const char *paed_path);

#endif // VIMMON_SCENE_VIEW_H
