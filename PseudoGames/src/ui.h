#ifndef UI_H
#define UI_H
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

// helpers de UI compartidos entre pantallas
void dibujadoTexto(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y);
void dibujadoTextoColor(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, SDL_Color color);

void dibujadoTextoMultilinea(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, int max_ancho);
void dibujadoTextoMultilineaColor(SDL_Renderer *renderer, TTF_Font *fuente, const char *texto, int x, int y, int max_ancho, SDL_Color color);

void screen_transition(SDL_Renderer *renderer, int ancho, int alto);
void screen_poweron(SDL_Renderer *renderer, int ancho, int alto);
void dibujarArandela(SDL_Renderer *renderer, int cx, int cy, int radio, SDL_Color color, SDL_Color bg);

// declaraciones de pantallas
int screenMenu(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto);
int screenDoc(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto);
int screenLvLs(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto);
int screenLvLEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num);
int screenSoluciones(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto);
int screenPomodoro(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto);
int screenConfig(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, SDL_Window *ventana);
int screenFeedback(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto);
int screenFreeEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num);
#endif
