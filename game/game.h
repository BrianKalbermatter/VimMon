#ifndef VIMMON_GAME_H
#define VIMMON_GAME_H

#include "../engine/engine.h"

// ============================================================
// TU JUEGO vive acá.
//
// El motor llama a game_setup(world) una sola vez, al arrancar
// (cuando escribís "engine" en la consola de vimmon). Adentro
// creás tus entidades con world_spawn() y les enchufás su lógica.
// Después el motor corre el loop solo. Vos NO tocás el motor:
// solo escribís este archivo.
// ============================================================
void game_setup(World *w);

#endif // VIMMON_GAME_H
