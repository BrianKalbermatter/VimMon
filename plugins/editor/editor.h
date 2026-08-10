#ifndef VIMMON_EDITOR_H
#define VIMMON_EDITOR_H

#include "../../bus/plugin.h"

// Plugin editor: conecta el editorBim al bus de VimMon.
//
// NO es el editor. El editor lo escribe el usuario y vive en
// paed/scripts/editorBim/. Este plugin solo hace de puente: escucha
// EVENT_EDITOR_OPEN, lanza el editor sobre el archivo que le pidieron, y
// cuando vuelve valida el resultado con el parser de PAED.
//
// Es el mismo patron que 'engine': el comando de la consola publica un evento,
// el plugin lo agarra y abre el programa. Asi el editor es una opcion mas del
// OS, como ai o help, y no un script suelto que se corre por afuera.

extern Plugin editor_plugin;

#endif // VIMMON_EDITOR_H
