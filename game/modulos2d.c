// Los juegos que el editor 2D puede abrir. Un juego nuevo se suma aca:
// su Modulo2D y nada mas, el editor no cambia.

#include "game/delirio/delirio.h"
#include "plugins/editor2d/editor2d.h"

Modulo2D *const modulos2d[] = {
  &delirio_modulo,
};
const int n_modulos2d = (int)(sizeof modulos2d / sizeof modulos2d[0]);
