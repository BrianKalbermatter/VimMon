#ifndef VIMMON_MODULO2D_H
#define VIMMON_MODULO2D_H

#include "plugins/renderer2d/renderer2d.h"

// ============================================================
// VimMon — CONTRATO DE MODULO 2D
//
// Un JUEGO para el motor. El juego deja de ser un programa con su propio
// main() y su propio loop: ofrece estas funciones y el que manda es el
// motor. Es la idea de Unity: el editor es dueno de la ventana y del
// tiempo, y llama al juego cuando apretas Play.
//
//   cargar      una vez al abrir el editor: texturas, atlas, datos
//   iniciar     en cada Play: partida nueva
//   actualizar  cada frame mientras corre (no en pausa)
//   dibujar     cada frame, en el lienzo del juego
//   terminar    en Stop
//   descargar   al cerrar el editor
//
// El mismo modulo corre en el editor o solo (game/delirio/main_delirio.c):
// el juego no sabe ni le importa quien lo esta llamando.
// ============================================================

// Lo que el juego necesita saber del jugador real. El motor ya lo tradujo
// al lienzo del juego: el juego nunca ve la ventana del editor.
typedef struct {
    float mouse_x, mouse_y;     // px del lienzo del juego
    int   mouse_dentro;         // el puntero esta sobre el juego
    int   boton[M2_COUNT];
    int   tecla[T2_COUNT];      // 0 si el juego no tiene el foco
} Entrada2D;

#define INSPECTOR_LINEAS 16
#define INSPECTOR_LARGO  64

// Un actor u objeto de la escena, para la Hierarchy y la vista Escena.
// La lista es plana; el arbol sale de `padre` (indice de otro objeto de la
// misma lista, siempre ANTERIOR a este, o -1 si es raiz).
#define ESCENA_MAX 512
typedef struct {
    char   nombre[40];      // "Jugador", "Enemigo 3", "Puerta norte"
    char   tipo[20];        // "Jugador", "Enemigo", "Puerta", "Grupo"...
    int    padre;
    Rect2D caja;            // donde esta, px de mundo. w == 0: no ocupa lugar
    int    movible;         // la vista Escena lo puede arrastrar
} Objeto2D;

#define SPRITES_MAX 64

typedef struct {
    const char *nombre;
    int alto_objetivo;          // px de arte que entran de alto (Delirio: 480)

    int  (*cargar)(const Renderer2D *r);           // 0 si salio bien
    void (*iniciar)(void);
    void (*actualizar)(const Entrada2D *e, float dt_ms, int ancho, int alto);
    // Dibuja con la camara que quiera sobre el destino que ya eligio el
    // motor. ancho x alto = tamano del lienzo del juego.
    void (*dibujar)(const Renderer2D *r, int ancho, int alto);
    void (*terminar)(void);
    void (*descargar)(const Renderer2D *r);

    // Lineas para el panel Inspector ("jugador  8848, 7148"). Devuelve
    // cuantas escribio.
    int  (*inspeccionar)(char lineas[][INSPECTOR_LARGO], int max);

    // ── Escena (todo opcional: NULL = el juego no lo ofrece) ──
    // Valen mientras hay partida (entre iniciar y terminar).
    int  (*escena)(Objeto2D *objs, int max);            // cuantos escribio
    int  (*inspeccionar_objeto)(int i, char lineas[][INSPECTOR_LARGO], int max);
    void (*mover_objeto)(int i, float x, float y);      // x, y = nuevo centro de la caja
    void (*mundo_tamano)(float *w, float *h);           // px
    // Dibuja la parte `visible` del mundo (px de mundo) SIN camara propia:
    // el motor ya fijo camara y zoom. Si el zoom es chico conviene dibujar
    // menos detalle (un mapa), porque entra mucho mundo.
    void (*dibujar_mundo)(const Renderer2D *r, Rect2D visible, float zoom);

    // PNGs que usa el juego, rutas absolutas, para la pizarra de sprites.
    int  (*sprites)(const char **rutas, int max);
} Modulo2D;

#endif // VIMMON_MODULO2D_H
