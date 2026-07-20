#ifndef VIMMON_KEYBOARD_H
#define VIMMON_KEYBOARD_H

#include "../../bus/plugin.h"

// Datos que viaja adentro del Event cuando se dispara EVENT_KEYBOARD
typedef struct {
    char tecla;
} KeyboardEvent;

extern Plugin keyboard_plugin;

#endif // VIMMON_KEYBOARD_H
