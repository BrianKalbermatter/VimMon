// playerC.c — el CUERPO de lo que playerC.h promete.
//
// Fijate que aca no hay un solo scanf. Este archivo no le pregunta nada a
// nadie: recibe ordenes ya digeridas y las ejecuta. Leer el teclado o el mouse
// es tarea del host (ver platform.h).

#include "playerC.h"
#include <math.h>

// Cuando falta menos que esto para llegar, se considera que llego. Sin un
// margen, el soldado se pasaria de largo y volveria, temblando alrededor del
// destino para siempre.
#define UMBRAL_LLEGADA 0.05f

void soldado_init(Soldado *s, float x, float y) {
  s->x = x;
  s->y = y;
  s->destino_x = x;
  s->destino_y = y;
  s->moviendose = 0;
  s->velocidad = 12.0f; // celdas por segundo
  s->seleccionado = 0;

  s->hp = s->max_hp = 100;
  s->vivo = 1;
  s->ataque = 8;
  s->cadencia = 0.6f;
  s->espera = 0.0f;
}

void soldado_ordenar_ir(Soldado *s, int x, int y) {
  s->destino_x = (float)x;
  s->destino_y = (float)y;
  s->moviendose = 1;
}

void soldado_update(Soldado *s, float dt) {
  if (!s->moviendose)
    return;

  // Vector que va de donde esta hacia donde tiene que ir.
  float dx = s->destino_x - s->x;
  float dy = s->destino_y - s->y;

  // Pitagoras: la distancia en linea recta. Es la hipotenusa del triangulo que
  // forman dx y dy.
  float distancia = sqrtf(dx * dx + dy * dy);

  if (distancia < UMBRAL_LLEGADA) {
    s->x = s->destino_x;
    s->y = s->destino_y;
    s->moviendose = 0;
    return;
  }

  // Cuanto avanza en ESTE frame. Multiplicar por dt es lo que hace que la
  // velocidad sea de 12 celdas por segundo de verdad, y no de 12 celdas por
  // frame: en una maquina que corre al doble de fps, dt es la mitad.
  float paso = s->velocidad * dt;

  // Si el paso se pasa del destino, lo clavamos ahi en vez de sobrepasarlo.
  if (paso >= distancia) {
    s->x = s->destino_x;
    s->y = s->destino_y;
    s->moviendose = 0;
    return;
  }

  // Dividir por la distancia deja un vector de largo 1 (normalizado): pura
  // direccion, sin magnitud. Recien ahi se lo multiplica por el paso. Sin
  // normalizar, un destino lejano lo movería mas rapido que uno cercano.
  s->x += (dx / distancia) * paso;
  s->y += (dy / distancia) * paso;
}

// --- Teclado (disponible, hoy sin usar) --------------------------------------
// Ver el comentario en playerC.h. No la llama nadie todavia, y esta bien asi.
//
// Fijate que no toca ningun Soldado ni conoce el framebuffer: solo traduce una
// tecla a una direccion. Por eso sirve igual para un soldado, para un cursor o
// para el menu que hagas manana.
int tecla_a_direccion(char tecla, int *dx, int *dy) {
  *dx = 0;
  *dy = 0;

  switch (tecla) {
  case 'w':
    *dy = -1;
    break;
  case 's':
    *dy = 1;
    break;
  case 'a':
    *dx = -1;
    break;
  case 'd':
    *dx = 1;
    break;
  default:
    return 0; // no era una tecla de movimiento
  }
  return 1;
}
