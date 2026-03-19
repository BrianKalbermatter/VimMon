#ifndef SCREEN_VERIFICADOR_H
#define SCREEN_VERIFICADOR_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "niveles.h"

/* Muestra la pantalla de verificacion para el nivel dado.
   El jugador ingresa su respuesta por cada caso de prueba.
   Retorna 1 si todos los casos pasan, 0 si falla alguno o escapa con ESC. */
int screenVerificador(SDL_Renderer *renderer, TTF_Font *fuente,
                      int ancho, int alto, Nivel *n);

#endif
