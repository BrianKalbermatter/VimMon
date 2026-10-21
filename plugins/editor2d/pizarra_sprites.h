// PIZARRA INFINITA de sprites: cada PNG del juego es un nodo sobre un
// lienzo sin bordes. Se navega como un mapa (arrastrar con el medio o el
// derecho, rueda para acercar hacia el mouse) y, de cerca, se pinta pixel
// por pixel.
//
// Herramientas (barra arriba de la pizarra, y teclas):
//   B lapiz   E goma   I gotero   arrastrar un nodo por su titulo lo mueve
//   Ctrl+Z deshacer   Ctrl+S guardar el sprite seleccionado (sobre su PNG)
// Paleta: los colores que mas usa el sprite elegido (hasta 32) + negro y blanco.
#ifndef VIMMON_PIZARRA_SPRITES_H
#define VIMMON_PIZARRA_SPRITES_H

#include "ui2d.h"

#define PIZARRA_NODOS 64
// Cada paso guarda el sprite entero: personaje_3 (1024x3648) son ~15 MB
// por paso, asi que son pocos.
#define PIZARRA_DESHACER 8

typedef struct {
    char      ruta[512];
    char      nombre[64];      // el archivo sin carpeta
    uint32_t *pix;             // ARGB en RAM: aca se pinta
    int       w, h;
    Textura2D tex;             // la copia en GPU, se actualiza al pintar
    float     x, y;            // esquina de arriba a la izquierda, px de pizarra
    int       modificado;      // hay cambios sin guardar
} NodoSprite;

typedef enum { HERR_LAPIZ, HERR_GOMA, HERR_GOTERO } Herramienta;

typedef struct {
    NodoSprite nodos[PIZARRA_NODOS];
    int   n_nodos;
    int   seleccion;           // nodo activo, -1 = ninguno
    float cam_x, cam_y, zoom;  // centro de la vista en px de pizarra
    Herramienta herramienta;
    uint32_t color;
    int   arrastrando;         // 0 nada, 1 camara, 2 nodo, 3 pintando
    float agarre_x, agarre_y;
    // Deshacer: copias completas del nodo antes de cada trazo (sprites de
    // pocos KB/MB: copiar es mas simple que guardar diferencias).
    struct { int nodo; uint32_t *pix; } deshacer[PIZARRA_DESHACER];
    int   n_deshacer;
    char  mensaje[128];        // "guardado personaje_3.png", se muestra un rato
    float mensaje_ms;
} Pizarra;

// Abre los PNG (los que no se pueden leer se saltean) y los acomoda en
// filas. Reemplaza lo que hubiera.
void pizarra_abrir(Pizarra *p, const Renderer2D *r, const char **rutas, int n);
void pizarra_cerrar(Pizarra *p, const Renderer2D *r);

// `e` en px del lienzo de la vista (w x h).
void pizarra_actualizar(Pizarra *p, const Renderer2D *r, const EntradaUI *e, int w, int h, float dt_ms);
void pizarra_dibujar(const Pizarra *p, const Renderer2D *r, int w, int h);

// Lineas para el Inspector cuando la pestana Sprites esta activa.
int pizarra_inspeccionar(const Pizarra *p, char lineas[][64], int max);

#endif
