#include "combate.h"
#include <math.h>

// A que distancia una bala se considera que dio en el blanco. Menos de una
// celda: la bala avanza 0.25 celdas por frame a 120 fps, asi que no hay riesgo
// de que atraviese a alguien sin tocarlo.
#define RADIO_IMPACTO 0.8f

void enemigo_init(Enemigo *e, float x, float y) {
  soldado_init(&e->cuerpo, x, y);
  e->cuerpo.hp = e->cuerpo.max_hp = 60;
  e->cuerpo.velocidad = 7.0f; // mas lento que un soldado (12)
  e->cuerpo.ataque = 5;
  e->cuerpo.cadencia = 0.9f;
  e->objetivo = -1;
  e->puesto_x = x; // hasta que le digan otra cosa, custodia donde nacio
  e->puesto_y = y;
}

void enemigo_vigilar(Enemigo *e, float x, float y) {
  e->puesto_x = x;
  e->puesto_y = y;
}

static float distancia(float ax, float ay, float bx, float by) {
  float dx = bx - ax;
  float dy = by - ay;
  return sqrtf(dx * dx + dy * dy);
}

// Aplica el danio y marca la muerte. Centralizado para que la vida no pueda
// quedar negativa: si no, la barra dibujaria una longitud negativa.
static void golpear(Soldado *victima, int danio) {
  victima->hp -= danio;
  if (victima->hp <= 0) {
    victima->hp = 0;
    victima->vivo = 0;
    victima->moviendose = 0;
  }
}

// Devuelve el indice del soldado vivo mas cercano, o -1 si no queda ninguno.
static int mas_cercano(float x, float y, const Soldado *ss, int cant) {
  int mejor = -1;
  float mejor_dist = 0.0f;

  for (int i = 0; i < cant; i++) {
    if (!ss[i].vivo)
      continue;
    float d = distancia(x, y, ss[i].x, ss[i].y);
    if (mejor == -1 || d < mejor_dist) {
      mejor = i;
      mejor_dist = d;
    }
  }
  return mejor;
}

void combate_enemigos(Enemigo *es, int cant_e, Soldado *ss, int cant_s,
                      Proyectil *ps, int cant_p, float dt) {
  for (int i = 0; i < cant_e; i++) {
    Enemigo *e = &es[i];
    if (!e->cuerpo.vivo)
      continue;

    // El temporizador baja siempre, este o no en rango. Asi el que acaba de
    // llegar al frente no tiene que esperar la cadencia completa.
    if (e->cuerpo.espera > 0.0f)
      e->cuerpo.espera -= dt;

    // Lo primero es el puesto, no el enemigo a la vista. Un guardia que se
    // corre a matar deja el tesoro solo, y eso es exactamente lo que no
    // queremos: el jugador podria sacarlo de posicion con un senuelo.
    float al_puesto =
        distancia(e->cuerpo.x, e->cuerpo.y, e->puesto_x, e->puesto_y);
    if (al_puesto > RADIO_GUARDIA)
      soldado_ordenar_ir(&e->cuerpo, (int)e->puesto_x, (int)e->puesto_y);
    else
      e->cuerpo.moviendose = 0;

    // Se recalcula el objetivo cada frame a proposito: si el soldado al que
    // apuntaba murio o se alejo, el guardia cambia de blanco solo.
    e->objetivo = mas_cercano(e->cuerpo.x, e->cuerpo.y, ss, cant_s);
    if (e->objetivo < 0)
      continue; // no hay nadie vivo a la vista

    Soldado *victima = &ss[e->objetivo];
    float d = distancia(e->cuerpo.x, e->cuerpo.y, victima->x, victima->y);

    // Dispara desde donde esta. Sin acercarse: el que decide la distancia de la
    // pelea es el jugador, que puede elegir entrar o quedarse afuera.
    if (d <= ALCANCE_ENEMIGO && e->cuerpo.espera <= 0.0f) {
      proyectil_disparar(ps, cant_p, e->cuerpo.x, e->cuerpo.y, victima->x,
                         victima->y, e->cuerpo.ataque, BANDO_ENEMIGO);
      e->cuerpo.espera = e->cuerpo.cadencia;
    }
  }
}

void combate_soldados(Soldado *ss, int cant_s, Enemigo *es, int cant_e,
                      Proyectil *ps, int cant_p, float dt) {
  for (int i = 0; i < cant_s; i++) {
    Soldado *s = &ss[i];
    if (!s->vivo)
      continue;

    if (s->espera > 0.0f)
      s->espera -= dt;
    if (s->espera > 0.0f)
      continue; // todavia esta recargando

    // Le tira al enemigo vivo mas cercano que tenga a tiro. Elegir el mas
    // cercano y no el primero del arreglo evita que dispare a uno lejano
    // teniendo otro encima.
    int blanco = -1;
    float mejor = 0.0f;
    for (int j = 0; j < cant_e; j++) {
      if (!es[j].cuerpo.vivo)
        continue;
      float d = distancia(s->x, s->y, es[j].cuerpo.x, es[j].cuerpo.y);
      if (d > ALCANCE_SOLDADO)
        continue;
      if (blanco == -1 || d < mejor) {
        blanco = j;
        mejor = d;
      }
    }

    if (blanco < 0)
      continue; // nada a tiro

    proyectil_disparar(ps, cant_p, s->x, s->y, es[blanco].cuerpo.x,
                       es[blanco].cuerpo.y, s->ataque, BANDO_SOLDADO);
    s->espera = s->cadencia;
  }
}

void combate_impactos(Proyectil *ps, int cant_p, Soldado *ss, int cant_s,
                      Enemigo *es, int cant_e) {
  for (int i = 0; i < cant_p; i++) {
    Proyectil *p = &ps[i];
    if (!p->activo)
      continue;

    // Una bala solo lastima al bando contrario. Sin este filtro, un soldado
    // que dispara con otro adelante lo mataria de una.
    if (p->bando == BANDO_SOLDADO) {
      for (int j = 0; j < cant_e; j++) {
        if (!es[j].cuerpo.vivo)
          continue;
        if (distancia(p->x, p->y, es[j].cuerpo.x, es[j].cuerpo.y) >
            RADIO_IMPACTO)
          continue;
        golpear(&es[j].cuerpo, p->danio);
        p->activo = 0; // la bala se consume en el primero que toca
        break;
      }
    } else {
      for (int j = 0; j < cant_s; j++) {
        if (!ss[j].vivo)
          continue;
        if (distancia(p->x, p->y, ss[j].x, ss[j].y) > RADIO_IMPACTO)
          continue;
        golpear(&ss[j], p->danio);
        p->activo = 0;
        break;
      }
    }
  }
}
