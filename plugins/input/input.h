#ifndef VIMMON_INPUT_H
#define VIMMON_INPUT_H

#include "../../bus/plugin.h"

// Datos que viajan adentro del Event cuando se dispara EVENT_KEYBOARD
typedef struct {
    char tecla;
} KeyboardEvent;

// Datos que viajan adentro del Event cuando se dispara EVENT_MOUSE
typedef struct {
    int x;
    int y;
    int boton;       // 0 = izquierdo, 1 = medio, 2 = derecho
    int presionado;  // 1 = press, 0 = release
} MouseEvent;

extern Plugin input_plugin;

#endif // VIMMON_INPUT_H
