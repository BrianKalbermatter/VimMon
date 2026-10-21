// Vista ESCENA y panel HIERARCHY del editor 2D, como en Unity.
//
// Escena: el mundo del juego con una camara LIBRE (no la del jugador):
// arrastrar con el boton del medio o el derecho mueve la vista, la rueda
// acerca y aleja hacia el mouse. Encima van las cajas de los objetos; clic
// selecciona, arrastrar con el izquierdo mueve el seleccionado si es
// movible. F encuadra el seleccionado.
//
// Hierarchy: el arbol de objetos de la escena (Objeto2D.padre). Clic
// selecciona, el triangulo pliega/despliega un grupo. La seleccion es la
// misma en los dos lados y la usa el Inspector.
#ifndef VIMMON_VISTA_ESCENA_H
#define VIMMON_VISTA_ESCENA_H

#include "modulo2d.h"
#include "ui2d.h"

typedef struct {
    float cam_x, cam_y;        // centro de la vista, px de mundo
    float zoom;
    int   seleccion;           // indice en objs, -1 = nada
    int   arrastrando;         // 0 nada, 1 moviendo la camara, 2 moviendo el objeto
    float agarre_x, agarre_y;  // donde se agarro (px de mundo), para arrastrar sin salto
    unsigned char plegado[ESCENA_MAX];
    int   scroll;              // filas de la Hierarchy corridas hacia arriba
    int   encuadrado;          // ya se centro la camara al arrancar la partida
    Objeto2D objs[ESCENA_MAX];
    int   n_objs;
} VistaEscena;

void escena_iniciar(VistaEscena *v);

// Una vez por frame mientras hay partida: relee los objetos del modulo.
void escena_refrescar(VistaEscena *v, const Modulo2D *m);

// La vista Escena. `e` en px del lienzo de la vista (w x h). Dibujar va
// despues de que el editor eligio el destino (el lienzo de la vista).
void escena_actualizar(VistaEscena *v, const Modulo2D *m, const EntradaUI *e, int w, int h);
void escena_dibujar(VistaEscena *v, const Modulo2D *m, const Renderer2D *r, int w, int h);

// El panel Hierarchy, en el rectangulo `zona` de la ventana. `e` en px de
// la ventana. Dibuja directo en la ventana.
void jerarquia_actualizar(VistaEscena *v, const EntradaUI *e, Rect2D zona, int alto_texto);
void jerarquia_dibujar(const VistaEscena *v, const Renderer2D *r, Rect2D zona);

#endif
