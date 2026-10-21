// El orden de cada frame es el de main.ts del cliente web:
//   entrada -> juego_actualizar -> personaje -> camara -> dibujar
//
// El navegador habla con el nucleo por web.c; aca no hace falta el puente:
// se llama directo a juego.h.

#include "delirio.h"

#include "hoja.h"
#include "juego.h"
#include "personaje.h"
#include "vista_laberinto.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef RUTA_PUBLIC
#define RUTA_PUBLIC "client/public"
#endif

#define FONDO RGB2(0x0d, 0x0b, 0x12)
// Filas vacias bajo los pies en el lienzo de personaje_3: se dibuja mas
// abajo para que los pies queden sobre la posicion.
#define PLAYER_FEET_PADDING 13
#define PLAYER_BODY_HEIGHT 62

// Lo que se ordena por profundidad: la Y de los pies, y para empates el
// tipo (DEPTH de depth.ts). double y no float: con el mundo de ~17.700 px
// un float no distingue y de y + 0.000001.
typedef enum { PROF_MURO = 0, PROF_PUERTA = 1, PROF_JUGADOR = 5 } TipoProf;
typedef struct {
  double y;
  TipoProf tipo;
  int dato;      // fila del muro, o indice de hoja (puerta * 2 + hoja)
} Dibujable;

static VistaLaberinto vista;
static Hoja hoja;
static Personaje p;
static Animador anim;
static Camara cam;
static float spawn_x, spawn_y;
static int corriendo;
static unsigned semilla_fija, semilla;
static int caminar_solo;
static float solo_dx, solo_dy;

void delirio_fijar_semilla(unsigned s) { semilla_fija = s; }

void delirio_caminar_solo(float dx, float dy) {
  caminar_solo = 1;
  solo_dx = dx;
  solo_dy = dy;
}

static int comparar(const void *a, const void *b) {
  const Dibujable *x = a, *y = b;
  if (x->y != y->y) return x->y < y->y ? -1 : 1;
  return (int)x->tipo - (int)y->tipo;
}

static int cargar(const Renderer2D *r) {
  if (vista_cargar(&vista, r, RUTA_PUBLIC "/assets/maze_tiles")) return 1;
  if (hoja_cargar(&hoja, r, RUTA_PUBLIC "/sprites/personaje_3")) return 1;
  return 0;
}

static void iniciar(void) {
  semilla = semilla_fija ? semilla_fija : (unsigned)time(NULL);
  juego_iniciar(semilla);
  const Entity *j = juego_jugador();
  spawn_x = j->posicion.x;
  spawn_y = j->posicion.y;
  vista_plaza(&vista, juego_laberinto(), spawn_x, spawn_y);

  personaje_iniciar(&p, spawn_x, spawn_y);
  personaje_sincronizar(&p, spawn_x, spawn_y, 0, 0, j->estados, 0, 0, 0, 0, 0);
  anim = (Animador){ .hoja = &hoja };
  char tag[48];
  animador_tocar(&anim, personaje_tag(&p, tag, sizeof tag), 1);
  cam = (Camara){ .x = p.x, .y = p.y - PLAYER_BODY_HEIGHT / 2.0f };
  corriendo = 0;
}

static void actualizar(const Entrada2D *e, float dt, int ancho, int alto) {
  float W = (float)ancho, H = (float)alto;
  cam.ancho = W;
  cam.alto = H;
  const Entity *j = juego_jugador();

  // Mantener el clic camina hacia el cursor (estilo Diablo), Shift corre.
  // El cursor pasa de pixel del lienzo a pixel del mundo.
  float wx = e->mouse_x + cam.x - W / 2, wy = e->mouse_y + cam.y - H / 2;
  if (e->mouse_dentro && e->boton[M2_IZQ]) juego_ir_a(wx, wy);
  if (caminar_solo) juego_ir_a(spawn_x + solo_dx, spawn_y + solo_dy);
  if (e->tecla[T2_SHIFT] != corriendo) {
    corriendo = e->tecla[T2_SHIFT];
    juego_mantener_habilidad(HAB_CORRER, corriendo);
  }

  juego_actualizar(dt);

  int estaba_muerto = p.movimiento == MOV_MUERTO;
  personaje_sincronizar(&p, j->posicion.x, j->posicion.y, j->dir.x, j->dir.y, j->estados, dt,
                        e->mouse_dentro, wx, wy, j->posicion.y - PLAYER_BODY_HEIGHT / 2.0f);
  if (estaba_muerto && p.movimiento != MOV_MUERTO) {
    cam.x = p.x; // reaparecio en la base: salta ahi en vez de deslizarse
    cam.y = p.y - PLAYER_BODY_HEIGHT / 2.0f;
  }
  char tag[48];
  animador_tocar(&anim, personaje_tag(&p, tag, sizeof tag), p.movimiento != MOV_MUERTO);
  animador_avanzar(&anim, dt, personaje_en_reversa(&p));

  const float mundo = (float)(LAB_TILES * LAB_TILE);
  camara_seguir(&cam, p.x, p.y - PLAYER_BODY_HEIGHT / 2.0f, p.mira, dt);
  camara_limitar(&cam, mundo, mundo, WALL_HEIGHT);
}

static void dibujar_jugador(const Renderer2D *r) {
  const Cuadro *c = animador_cuadro(&anim);
  if (!c) return;
  float pies = p.y + PLAYER_FEET_PADDING;
  // El ancla es el centro del cuadro, como placeFrame.
  Rect2D d = { roundf(p.x - c->origen.w / 2), roundf(pies - c->origen.h), c->origen.w, c->origen.h };
  r->sprite(hoja.textura, c->origen, d, BLANCO2, 0);
}

static void dibujar(const Renderer2D *r, int ancho, int alto) {
  float W = (float)ancho, H = (float)alto;
  static Dibujable lista[LAB_TILES + 16];

  r->rect((Rect2D){ 0, 0, W, H }, FONDO);
  // Redondeado, como camera.apply: el pixel art no tolera medio pixel.
  r->camara(-roundf(W / 2 - cam.x), -roundf(H / 2 - cam.y));
  Rect2D vis = { cam.x - W / 2, cam.y - H / 2, W, H };
  const Laberinto *lab = juego_laberinto();
  vista_piso(&vista, r, lab, vis);

  int c0, c1, r0, r1, n = 0;
  vista_celdas(vis, &c0, &c1, &r0, &r1);
  for (int fila = r0; fila <= r1; fila++)
    lista[n++] = (Dibujable){ (double)((fila + 1) * LAB_TILE), PROF_MURO, fila };
  RectPx hojas[8];
  for (int i = 0; i < 4; i++) {
    laberinto_hojas(lab, i, &hojas[i * 2]);
    for (int k = 0; k < 2; k++) {
      RectPx h = hojas[i * 2 + k];
      if (h.ancho <= 0.5f || h.alto <= 0.5f) continue;
      lista[n++] = (Dibujable){ roundf(h.y + h.alto), PROF_PUERTA, i * 2 + k };
    }
  }
  lista[n++] = (Dibujable){ p.y, PROF_JUGADOR, 0 };
  qsort(lista, (size_t)n, sizeof lista[0], comparar);

  for (int i = 0; i < n; i++) {
    switch (lista[i].tipo) {
      case PROF_MURO: vista_fila_muros(&vista, r, lab, lista[i].dato, c0, c1); break;
      case PROF_PUERTA: vista_hoja_puerta(&vista, r, hojas[lista[i].dato]); break;
      case PROF_JUGADOR: dibujar_jugador(r); break;
    }
  }
  r->camara(0, 0);
}

static void terminar(void) {
  if (corriendo) juego_mantener_habilidad(HAB_CORRER, 0);
  corriendo = 0;
}

static void descargar(const Renderer2D *r) {
  vista_descargar(&vista, r);
  r->textura_liberar(hoja.textura);
  free(hoja.cuadros);
  free(hoja.anims);
}

static int inspeccionar(char l[][INSPECTOR_LARGO], int max) {
  static const char *ESTADOS[] = { "QUIETO", "CAMINANDO", "ATACANDO", "CORRIENDO", "MUERTO" };
  const Entity *j = juego_jugador();
  const Jugador *jc = juego_jugador_completo();
  const Laberinto *lab = juego_laberinto();
  int n = 0;
#define LINEA(...) if (n < max) snprintf(l[n++], INSPECTOR_LARGO, __VA_ARGS__)
  LINEA("semilla    %u", semilla);
  LINEA("jugador    %.0f, %.0f", j->posicion.x, j->posicion.y);
  LINEA("estado     %s", (unsigned)j->estados < 5 ? ESTADOS[j->estados] : "?");
  LINEA("vida       %d / %d", j->vida, j->vidaMAX);
  LINEA("muertes    %d", jc->muertes);
  LINEA("mira       %s", direccion_nombre(p.mira));
  LINEA("animacion  %s", anim.anim ? anim.anim->nombre : "-");
  LINEA("camara     %.0f, %.0f", cam.x, cam.y);
  LINEA("puertas    %.0f%% abiertas", lab->apertura * 100);
  LINEA("laberinto  version %d", lab->version);
#undef LINEA
  return n;
}

Modulo2D delirio_modulo = {
  .nombre = "Delirio",
  .alto_objetivo = 480, // como TARGET_VIEW_HEIGHT del navegador
  .cargar = cargar,
  .iniciar = iniciar,
  .actualizar = actualizar,
  .dibujar = dibujar,
  .terminar = terminar,
  .descargar = descargar,
  .inspeccionar = inspeccionar,
};
