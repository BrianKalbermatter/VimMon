#ifndef VIMMON_EDITOR_H
#define VIMMON_EDITOR_H

#include "../../bus/plugin.h"

// Plugin editor: conecta PseudoGames al bus de VimMon.
//
// NO es el editor. PseudoGames es el IDE completo del usuario — menu, niveles,
// wiki, pomodoro y el editor adentro — y vive en paed/, con su propio Makefile.
// Este plugin solo hace de puente: escucha EVENT_EDITOR_OPEN y lo lanza.
//
// Es el mismo patron que 'engine': el comando de la consola publica un evento,
// el plugin lo agarra y abre el programa. Asi el IDE es una opcion mas del OS,
// como ai o help, y no algo que se corre por afuera.

extern Plugin editor_plugin;

#endif // VIMMON_EDITOR_H
