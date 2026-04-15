#ifndef SCREEN1_H
#define SCREEN1_H

#include <SDL3/SDL.h>

#define MAX_COMANDOS 256

// Estructura que guarda un comando del protocolo PAED → C
// tipo: "EJE" o "LINEA"
// x1,y1 → punto de inicio | x2,y2 → punto de fin
typedef struct {
    char  tipo[16];
    float x1, y1;
    float x2, y2;
} Comando;

// Ejecuta recta.paed con el interprete PAED y carga los comandos en el arreglo
void cargar_paed(const char* archivo, Comando* cmds, int* n);

// Dibuja todos los comandos cargados usando SDL_RenderLine
void dibujar_comandos(SDL_Renderer* renderer, Comando* cmds, int n);

#endif
