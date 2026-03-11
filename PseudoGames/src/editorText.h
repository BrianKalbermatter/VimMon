#ifndef EDITOR_TEXT_H
#define EDITOR_TEXT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

/* Dibuja el editor dentro de un rect sin loop propio (para embeber) */
void drawEditorText(SDL_Renderer *renderer, TTF_Font *fuente, SDL_Rect area);

/* Pantalla completa con loop propio.
 * nombre_fijo : si no es NULL/""  → carga saves/<nombre>.paed y F9 sobreescribe directo.
 * cons_titulo / cons_texto : si no son NULL → muestra consigna en panel izquierdo
 *                            y el editor ocupa la mitad derecha. */
int screenEditorText(SDL_Renderer *renderer, TTF_Font *fuente,
                     int ancho, int alto, const char *nombre_fijo,
                     const char *cons_titulo, const char *cons_texto);

#endif
