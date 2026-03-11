#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include "ui.h"
#include "niveles.h"
#include "progreso.h"
#include "editorText.h"

int
screenLvLEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num)
{
    Nivel *n = obtener_nivel(nivel_num);
    if (!n) return 0;

    char nombre[32];
    snprintf(nombre, sizeof(nombre), "nivel_%d", nivel_num);

    /* Editor con consigna a la izquierda y editor a la derecha */
    screenEditorText(renderer, fuente, ancho, alto, nombre, n->titulo, n->enunciado);

    marcar_completado(nivel_num);
    return 0;
}
