// Lo que el editor le pasa a cada vista (Escena, Sprites): el mouse en
// coordenadas del lienzo de la vista, con flancos ya calculados, y la
// rueda. Asi cada vista no vuelve a preguntarle todo al renderer.
#ifndef VIMMON_UI2D_H
#define VIMMON_UI2D_H

#include "plugins/renderer2d/renderer2d.h"

typedef struct {
    float mx, my;              // mouse en px del lienzo de la vista
    int   dentro;              // el mouse esta sobre la vista
    int   boton[M2_COUNT];     // apretado ahora
    int   apreto[M2_COUNT];    // se apreto ESTE frame
    int   solto[M2_COUNT];     // se solto ESTE frame
    float rueda;               // + = hacia adelante
    int   tecla[T2_COUNT];     // apretada ahora
    int   tecla_apreto[T2_COUNT]; // se apreto ESTE frame
} EntradaUI;

// Paleta del editor (la misma en todas las vistas).
#define UI_FONDO     RGB2(0x14, 0x11, 0x19)
#define UI_PANEL     RGB2(0x1e, 0x1a, 0x25)
#define UI_BORDE     RGB2(0x2e, 0x22, 0x2f)
#define UI_TEXTO     RGB2(0xc7, 0xc2, 0xb8)
#define UI_TENUE     RGB2(0x6b, 0x63, 0x6f)
#define UI_ACENTO    RGB2(0xfb, 0xb9, 0x54)
#define UI_BOTON     RGB2(0x2b, 0x25, 0x33)
#define UI_BOTON_ON  RGB2(0x45, 0x3a, 0x52)
#define UI_VACIO     RGB2(0x0d, 0x0b, 0x12)

#endif
