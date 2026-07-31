#include "proyectil.h"
#include <math.h>

#define VELOCIDAD_BALA 30.0f

int proyectil_disparar(Proyectil *ps, int cant, float x, float y, float ox,
                       float oy, int danio, int bando) {
  float dx = ox - x;
  float dy = oy - y;
  float dist = sqrtf(dx * dx + dy * dy);

  // Disparar contra uno mismo dejaria una direccion (0,0) y una division por
  // cero. La bala nunca se movería y quedaria clavada.
  if (dist < 0.0001f)
    return 0;

  for (int i = 0; i < cant; i++) {
    if (ps[i].activo)
      continue;

    ps[i].x = x;
    ps[i].y = y;
    ps[i].dx = dx / dist; // normalizado: la velocidad la pone velocidad, no la
    ps[i].dy = dy / dist; // distancia al blanco
    ps[i].velocidad = VELOCIDAD_BALA;
    ps[i].danio = danio;
    ps[i].bando = bando;
    ps[i].activo = 1;
    return 1;
  }
  return 0; // el aire esta lleno de balas; este disparo se pierde
}

void proyectiles_update(Proyectil *ps, int cant, float dt, int x0, int y0,
                        int x1, int y1) {
  for (int i = 0; i < cant; i++) {
    if (!ps[i].activo)
      continue;

    ps[i].x += ps[i].dx * ps[i].velocidad * dt;
    ps[i].y += ps[i].dy * ps[i].velocidad * dt;

    if (ps[i].x < x0 || ps[i].x > x1 || ps[i].y < y0 || ps[i].y > y1)
      ps[i].activo = 0;
  }
}
