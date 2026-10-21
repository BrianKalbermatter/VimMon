// Delirio como modulo del motor: el nucleo en C del juego (el mismo que
// corre en el navegador) mas su presentacion sobre renderer2d.
#ifndef DELIRIO_MODULO_H
#define DELIRIO_MODULO_H

#include "plugins/editor2d/modulo2d.h"

extern Modulo2D delirio_modulo;

// Opciones para probar sin jugar a mano. Valen para el proximo iniciar().
void delirio_fijar_semilla(unsigned semilla);    // 0 = al azar
// Camina solo hacia spawn + (dx, dy), como si mantuvieras el clic ahi.
void delirio_caminar_solo(float dx, float dy);

#endif
