#ifndef UI_H
#define UI_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// helpers de UI compartidos entre pantallas
void dibujadoTexto(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y);

void dibujadoTextoMultilinea(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, int max_ancho);

#endif
