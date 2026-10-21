// Editor 2D de VimMon: la ventana del motor. Abre un juego (Modulo2D), lo
// muestra en el centro y lo corre con Play / Pausa / Stop, como Unity.
#ifndef VIMMON_EDITOR2D_H
#define VIMMON_EDITOR2D_H

#include "modulo2d.h"

// La lista vive en game/modulos2d.c: el editor no conoce ningun juego.
extern Modulo2D *const modulos2d[];
extern const int n_modulos2d;

#endif
