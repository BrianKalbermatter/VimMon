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

#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef RUTA_ASSETS
#define RUTA_ASSETS "game/delirio/assets"
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
static Rect2D caja_plaza;   // px de mundo, la caja de las celdas de la plaza
static int teclas_antes[T2_COUNT];
static int bloqueando;
static int activa_antes[HAB_CANTIDAD]; // para ver que habilidad arranco este frame
static int auto_ataques_vistos;

// Teclas de habilidad, como ability-keys.ts del navegador.
static const struct { Tecla2D tecla; HabilidadId hab; } TECLAS_HAB[] = {
  { T2_Q, HAB_ATAQUE_LIGERO }, { T2_W, HAB_ATAQUE_RAPIDO }, { T2_E, HAB_ATAQUE_LARGO },
  { T2_R, HAB_ATAQUE_CRITICO }, { T2_A, HAB_APOYO }, { T2_S, HAB_PARRY },
  { T2_D, HAB_ULTI }, { T2_SPACE, HAB_SALTAR },
};

// Cuanto se muestra cada ataque: lo que esta activo en el nucleo
// (habilidades en src/mecanicas/jugador.c), pero nunca tan poco que no se vea.
static const float ATAQUE_ACTIVO_MS[] = { 200, 120, 400, 300 }; // ligero, rapido, largo, critico
#define MIN_SWING_MS 260.0f
// Golpear el piso (la ulti, D): lo que dura HAB_ULTI en el nucleo.
#define SLAM_MS 900.0f

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
  if (vista_cargar(&vista, r, RUTA_ASSETS "/tiles/maze_tiles")) return 1;
  if (hoja_cargar(&hoja, r, RUTA_ASSETS "/sprites/personaje_3")) return 1;
  return 0;
}

static void iniciar(void) {
  semilla = semilla_fija ? semilla_fija : (unsigned)time(NULL);
  juego_iniciar(semilla);
  const Entity *j = juego_jugador();
  spawn_x = j->posicion.x;
  spawn_y = j->posicion.y;
  vista_plaza(&vista, juego_laberinto(), spawn_x, spawn_y);

  // La caja de la plaza, para la Hierarchy: se busca una vez por partida.
  int c0 = LAB_TILES, c1 = -1, f0 = LAB_TILES, f1 = -1;
  for (int f = 0; vista.plaza && f < LAB_TILES; f++) {
    for (int c = 0; c < LAB_TILES; c++) {
      if (!vista.plaza[f * LAB_TILES + c]) continue;
      if (c < c0) c0 = c;
      if (c > c1) c1 = c;
      if (f < f0) f0 = f;
      if (f > f1) f1 = f;
    }
  }
  caja_plaza = c1 < 0 ? (Rect2D){ 0 }
                      : (Rect2D){ (float)(c0 * LAB_TILE), (float)(f0 * LAB_TILE),
                                  (float)((c1 - c0 + 1) * LAB_TILE), (float)((f1 - f0 + 1) * LAB_TILE) };

  personaje_iniciar(&p, spawn_x, spawn_y);
  personaje_sincronizar(&p, spawn_x, spawn_y, 0, 0, j->estados, 0, 0, 0, 0, 0, 0);
  anim = (Animador){ .hoja = &hoja };
  char tag[48];
  animador_tocar(&anim, personaje_tag(&p, tag, sizeof tag), 1);
  cam = (Camara){ .x = p.x, .y = p.y - PLAYER_BODY_HEIGHT / 2.0f };
  corriendo = 0;
  bloqueando = 0;
  memset(teclas_antes, 0, sizeof teclas_antes);
  memset(activa_antes, 0, sizeof activa_antes);
  auto_ataques_vistos = juego_auto_ataques();
}

// Las habilidades que arrancaron este frame ponen su animacion: atacar (en
// el aire, con las piernas recogidas), o golpear el piso. Igual que el
// recorrido de abilityEffects.update en main.ts.
static void animar_habilidades(void) {
  const Jugador *jc = juego_jugador_completo();
  const Entity *j = juego_jugador();
  int auto_atacado = juego_auto_ataques() > auto_ataques_vistos;
  auto_ataques_vistos = juego_auto_ataques();
  for (int h = 0; h < HAB_CANTIDAD; h++) {
    int activa = jugador_activa(jc, (HabilidadId)h);
    int arranco = activa && !activa_antes[h];
    activa_antes[h] = activa;
    if (!arranco || p.movimiento == MOV_MUERTO) continue;
    if (h <= HAB_ATAQUE_CRITICO) {
      const char *nombre = p.salto_ms >= 0 ? "jump_attack" : "attack";
      float ms = ATAQUE_ACTIVO_MS[h] > MIN_SWING_MS ? ATAQUE_ACTIVO_MS[h] : MIN_SWING_MS;
      char tag[32];
      snprintf(tag, sizeof tag, "%s_right", nombre);
      // El auto-ataque pega hacia donde camina, no hacia el cursor.
      int hacia = auto_atacado && h == HAB_ATAQUE_LIGERO && (j->dir.x != 0 || j->dir.y != 0)
                    ? direccion_de_vector(j->dir.x, j->dir.y) : -1;
      personaje_accion(&p, nombre, ms, hoja_duracion(&hoja, tag), hacia);
      animador_reiniciar(&anim);
    }
    if (h == HAB_ULTI && p.salto_ms < 0) {
      personaje_accion(&p, "slam", SLAM_MS, hoja_duracion(&hoja, "slam_right"), -1);
      animador_reiniciar(&anim);
    }
  }
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
  // Clic derecho mantenido bloquea. Las demas habilidades se usan al
  // apretar la tecla (no mientras se mantiene).
  int quiere_bloquear = e->mouse_dentro && e->boton[M2_DER];
  if (quiere_bloquear != bloqueando) {
    bloqueando = quiere_bloquear;
    juego_mantener_habilidad(HAB_BLOQUEAR, bloqueando);
  }
  for (size_t i = 0; i < sizeof TECLAS_HAB / sizeof TECLAS_HAB[0]; i++)
    if (e->tecla[TECLAS_HAB[i].tecla] && !teclas_antes[TECLAS_HAB[i].tecla])
      juego_usar_habilidad(TECLAS_HAB[i].hab);
  memcpy(teclas_antes, e->tecla, sizeof teclas_antes);

  juego_actualizar(dt);

  int estaba_muerto = p.movimiento == MOV_MUERTO;
  int saltando = jugador_activa(juego_jugador_completo(), HAB_SALTAR);
  personaje_sincronizar(&p, j->posicion.x, j->posicion.y, j->dir.x, j->dir.y, j->estados, saltando,
                        dt, e->mouse_dentro, wx, wy, j->posicion.y - PLAYER_BODY_HEIGHT / 2.0f);
  if (estaba_muerto && p.movimiento != MOV_MUERTO) {
    cam.x = p.x; // reaparecio en la base: salta ahi en vez de deslizarse
    cam.y = p.y - PLAYER_BODY_HEIGHT / 2.0f;
  }
  animar_habilidades();

  // Las acciones y la muerte se muestran una vez; el resto da vueltas. El
  // salto no avanza con el tiempo: sigue el reloj del salto del nucleo.
  char tag[48];
  int una_vez = p.movimiento == MOV_MUERTO || personaje_actuando(&p);
  animador_tocar(&anim, personaje_tag(&p, tag, sizeof tag), !una_vez);
  float salto_arte = personaje_salto_arte_ms(&p);
  if (salto_arte >= 0 && !personaje_actuando(&p)) animador_buscar(&anim, salto_arte);
  else animador_avanzar(&anim, dt, personaje_velocidad(&p), personaje_en_reversa(&p));

  const float mundo = (float)(LAB_TILES * LAB_TILE);
  camara_seguir(&cam, p.x, p.y - PLAYER_BODY_HEIGHT / 2.0f, p.mira, dt);
  camara_limitar(&cam, mundo, mundo, WALL_HEIGHT);
}

static void dibujar_jugador(const Renderer2D *r) {
  const Cuadro *c = animador_cuadro(&anim);
  if (!c) return;
  float piso = p.y + PLAYER_FEET_PADDING;
  float pies = piso - (float)personaje_altura(&p);
  // En el aire, una sombra en el piso que se achica cuanto mas alto esta.
  if (personaje_altura(&p) > 0) {
    float esc = 1.0f - (float)personaje_altura(&p) / 80.0f;
    float w = roundf(22 * esc), h = roundf(6 * esc);
    r->rect((Rect2D){ roundf(p.x - w / 2), roundf(p.y - h / 2), w, h }, ARGB(0x60, 0x0d, 0x0b, 0x12));
    r->rect((Rect2D){ roundf(p.x - w / 2 + 2), roundf(p.y - h / 2 - 1), w - 4, h + 2 }, ARGB(0x60, 0x0d, 0x0b, 0x12));
  }
  // El ancla es el centro del cuadro, como placeFrame.
  Rect2D d = { roundf(p.x - c->origen.w / 2), roundf(pies - c->origen.h), c->origen.w, c->origen.h };
  r->sprite(hoja.textura, c->origen, d, BLANCO2, 0);
}

// El mundo con todo su detalle, en la parte `vis` (px de mundo), con la
// camara que ya este puesta: el piso abajo de todo, y encima muros, hojas
// de puerta y jugador ordenados por la Y de los pies. La usan el juego y la
// vista Escena del editor.
static void dibujar_detalle(const Renderer2D *r, Rect2D vis) {
  static Dibujable lista[LAB_TILES + 16];
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
}

static void dibujar(const Renderer2D *r, int ancho, int alto) {
  float W = (float)ancho, H = (float)alto;
  r->rect((Rect2D){ 0, 0, W, H }, FONDO);
  // Redondeado, como camera.apply: el pixel art no tolera medio pixel.
  r->camara(-roundf(W / 2 - cam.x), -roundf(H / 2 - cam.y));
  dibujar_detalle(r, (Rect2D){ cam.x - W / 2, cam.y - H / 2, W, H });
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

static const char *ESTADOS[] = { "QUIETO", "CAMINANDO", "ATACANDO", "CORRIENDO", "MUERTO" };
static const char *estado_nombre(Estado e) { return (unsigned)e < 5 ? ESTADOS[e] : "?"; }

static int inspeccionar(char l[][INSPECTOR_LARGO], int max) {
  const Entity *j = juego_jugador();
  const Jugador *jc = juego_jugador_completo();
  const Laberinto *lab = juego_laberinto();
  int n = 0;
#define LINEA(...) if (n < max) snprintf(l[n++], INSPECTOR_LARGO, __VA_ARGS__)
  LINEA("semilla    %u", semilla);
  LINEA("jugador    %.0f, %.0f", j->posicion.x, j->posicion.y);
  LINEA("estado     %s", estado_nombre(j->estados));
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

// ── Escena: lo que ven la Hierarchy, la vista Escena y el Inspector ──

typedef enum {
  OBJ_LABERINTO, OBJ_PLAZA, OBJ_PUERTAS, OBJ_PUERTA, OBJ_JUGADOR, OBJ_MEDUSA, OBJ_ENEMIGOS, OBJ_ENEMIGO
} TipoObjeto;

// Que es cada objeto de la lista: el tipo y cual (puerta 0..3, hueco de la
// lista de enemigos).
typedef struct {
  TipoObjeto tipo;
  int dato;
} RefObjeto;

static const char *PUERTAS[4] = { "norte", "sur", "oeste", "este" };

// La caja de alguien parado en `pies`: el cuerpo entero, arriba de los pies.
static Rect2D caja_cuerpo(Direccion pies) {
  return (Rect2D){ pies.x - 16, pies.y - PLAYER_BODY_HEIGHT, 32, PLAYER_BODY_HEIGHT };
}

static Rect2D caja_puerta(const Laberinto *lab, int i) {
  RectTiles t = lab->puertas[i];
  return (Rect2D){ (float)(t.col * LAB_TILE), (float)(t.fila * LAB_TILE),
                   (float)(t.ancho * LAB_TILE), (float)(t.alto * LAB_TILE) };
}

static Rect2D unir(Rect2D a, Rect2D b) {
  if (a.w <= 0) return b;
  float x0 = fminf(a.x, b.x), y0 = fminf(a.y, b.y);
  float x1 = fmaxf(a.x + a.w, b.x + b.w), y1 = fmaxf(a.y + a.h, b.y + b.h);
  return (Rect2D){ x0, y0, x1 - x0, y1 - y0 };
}

// Arma la lista entera desde el estado del nucleo. La llaman escena(),
// inspeccionar_objeto() y mover_objeto(): los tres ven el mismo indice
// para el mismo objeto, porque la lista sale siempre en el mismo orden.
static int armar_escena(Objeto2D *objs, RefObjeto *refs, int max) {
  const Laberinto *lab = juego_laberinto();
  const Enemigos *en = juego_enemigos();
  const float mundo = (float)(LAB_TILES * LAB_TILE);
  int n = 0;
#define OBJETO(padre_, tipo_, dato_, caja_, movible_, tipo_nombre, ...)                           \
  do {                                                                                          \
    if (n >= max) return n;                                                                     \
    objs[n] = (Objeto2D){ .padre = (padre_), .caja = (caja_), .movible = (movible_) };          \
    snprintf(objs[n].nombre, sizeof objs[n].nombre, __VA_ARGS__);                               \
    snprintf(objs[n].tipo, sizeof objs[n].tipo, "%s", tipo_nombre);                            \
    refs[n] = (RefObjeto){ tipo_, dato_ };                                                      \
    n++;                                                                                        \
  } while (0)

  OBJETO(-1, OBJ_LABERINTO, 0, ((Rect2D){ 0, 0, mundo, mundo }), 0, "Grupo", "Laberinto");
  OBJETO(0, OBJ_PLAZA, 0, caja_plaza, 0, "Plaza", "Plaza");
  Rect2D todas = { 0 };
  for (int i = 0; i < 4; i++) todas = unir(todas, caja_puerta(lab, i));
  int puertas = n;
  OBJETO(0, OBJ_PUERTAS, 0, todas, 0, "Grupo", "Puertas");
  for (int i = 0; i < 4; i++) OBJETO(puertas, OBJ_PUERTA, i, caja_puerta(lab, i), 0, "Puerta", "Puerta %s", PUERTAS[i]);

  OBJETO(-1, OBJ_JUGADOR, 0, caja_cuerpo(juego_jugador()->posicion), 1, "Jugador", "Jugador");
  OBJETO(-1, OBJ_MEDUSA, 0, caja_cuerpo(juego_medusa()->posicion), 1, "Medusa", "Medusa");
  int grupo = n;
  OBJETO(-1, OBJ_ENEMIGOS, 0, ((Rect2D){ 0 }), 0, "Grupo", "Enemigos");
  for (int i = 0; i < ENEMIGOS_MAX; i++) {
    if (!en->lista[i].activo) continue;
    OBJETO(grupo, OBJ_ENEMIGO, i, caja_cuerpo(en->lista[i].entidad.posicion), 1, "Enemigo", "Enemigo %d", i);
  }
#undef OBJETO
  return n;
}

static Objeto2D tmp_objs[ESCENA_MAX];
static RefObjeto tmp_refs[ESCENA_MAX];

static int escena(Objeto2D *objs, int max) {
  if (max > ESCENA_MAX) max = ESCENA_MAX;
  return armar_escena(objs, tmp_refs, max);
}

static int inspeccionar_objeto(int i, char l[][INSPECTOR_LARGO], int max) {
  int total = armar_escena(tmp_objs, tmp_refs, ESCENA_MAX);
  if (i < 0 || i >= total) return 0;
  const Laberinto *lab = juego_laberinto();
  const Objeto2D *o = &tmp_objs[i];
  int n = 0;
#define LINEA(...) if (n < max) snprintf(l[n++], INSPECTOR_LARGO, __VA_ARGS__)
  LINEA("%s  (%s)", o->nombre, o->tipo);
  const Entity *e = NULL;
  switch (tmp_refs[i].tipo) {
    case OBJ_LABERINTO:
      LINEA("semilla    %u", semilla);
      LINEA("version    %d", lab->version);
      LINEA("celdas     %d..%d x %d..%d", lab->c_min, lab->c_max, lab->f_min, lab->f_max);
      LINEA("tiles      %d x %d de %d px", LAB_TILES, LAB_TILES, LAB_TILE);
      LINEA("obstaculos %d", lab->cant_obstaculos);
      break;
    case OBJ_PLAZA:
      LINEA("caja       %.0f, %.0f  %.0f x %.0f", o->caja.x, o->caja.y, o->caja.w, o->caja.h);
      break;
    case OBJ_PUERTA: {
      RectTiles t = lab->puertas[tmp_refs[i].dato];
      LINEA("apertura   %.0f%%", lab->apertura * 100);
      LINEA("tiles      col %d fila %d  %d x %d", t.col, t.fila, t.ancho, t.alto);
      LINEA("caja       %.0f, %.0f  %.0f x %.0f", o->caja.x, o->caja.y, o->caja.w, o->caja.h);
      break;
    }
    case OBJ_JUGADOR: e = juego_jugador(); break;
    case OBJ_MEDUSA: e = juego_medusa(); break;
    case OBJ_ENEMIGO: e = &juego_enemigos()->lista[tmp_refs[i].dato].entidad; break;
    case OBJ_PUERTAS:
    case OBJ_ENEMIGOS: {
      int hijos = 0;
      for (int k = i + 1; k < total; k++) hijos += tmp_objs[k].padre == i;
      LINEA("hijos      %d", hijos);
      break;
    }
  }
  if (e) {
    LINEA("pies       %.0f, %.0f", e->posicion.x, e->posicion.y);
    LINEA("vida       %d / %d", e->vida, e->vidaMAX);
    LINEA("estado     %s", estado_nombre(e->estados));
  }
  if (tmp_refs[i].tipo == OBJ_JUGADOR) LINEA("muertes    %d", juego_jugador_completo()->muertes);
#undef LINEA
  return n;
}

// Solo para el editor: el nucleo da punteros const para LEER, pero las
// entidades no estan definidas const (son variables comunes de main.c), asi
// que sacarles el const para escribir es legal en C. Fuera del editor nadie
// mueve a nadie de un salto.
static void mover_objeto(int i, float x, float y) {
  int total = armar_escena(tmp_objs, tmp_refs, ESCENA_MAX);
  if (i < 0 || i >= total || !tmp_objs[i].movible) return;
  // (x, y) es el centro de la caja; los pies estan en su borde de abajo.
  Direccion pies = { x, y + PLAYER_BODY_HEIGHT / 2.0f };
  Entity *e = NULL;
  switch (tmp_refs[i].tipo) {
    case OBJ_JUGADOR: e = (Entity *)juego_jugador(); break;
    case OBJ_MEDUSA: e = (Entity *)juego_medusa(); break;
    case OBJ_ENEMIGO: e = &((Enemigos *)juego_enemigos())->lista[tmp_refs[i].dato].entidad; break;
    default: return;
  }
  e->posicion = pies;
  if (tmp_refs[i].tipo == OBJ_JUGADOR) {
    juego_ir_a(pies.x, pies.y); // si no, sigue caminando al destino viejo
    p.x = pies.x;               // y se ve donde quedo aunque el juego este en pausa
    p.y = pies.y;
  }
}

static void mundo_tamano(float *w, float *h) {
  *w = *h = (float)(LAB_TILES * LAB_TILE);
}

// Debajo de este zoom un tile mide menos de 16 px en pantalla: se dibuja el
// mapa. Y aunque el zoom alcance, si la vista tiene demasiadas celdas (una
// ventana enorme) tambien: cada celda son hasta ~6 cuadrilateros y el
// renderer acepta 65536 por frame.
#define ZOOM_DETALLE 0.5f
#define CELDAS_DETALLE 10000

static void dibujar_mundo(const Renderer2D *r, Rect2D visible, float zoom) {
  float celdas = (visible.w / LAB_TILE + 1) * (visible.h / LAB_TILE + 4);
  if (zoom >= ZOOM_DETALLE && celdas <= CELDAS_DETALLE) {
    dibujar_detalle(r, visible);
    return;
  }
  vista_mapa(&vista, r, juego_laberinto());
  // El jugador, un punto que mide lo mismo en pantalla con cualquier zoom.
  float lado = fmaxf(LAB_TILE, 6 / zoom), borde = 1 / zoom;
  r->rect((Rect2D){ p.x - lado / 2 - borde, p.y - lado / 2 - borde, lado + 2 * borde, lado + 2 * borde },
          RGB2(0x2e, 0x22, 0x2f));
  r->rect((Rect2D){ p.x - lado / 2, p.y - lado / 2, lado, lado }, RGB2(0xff, 0xe0, 0x6a));
}

// Los PNG de una carpeta, ordenados por nombre, agregados a `rutas`.
static int comparar_nombres(const void *a, const void *b) { return strcmp(a, b); }

static int juntar_pngs(const char *carpeta, char rutas[][512], int n, int max) {
  DIR *d = opendir(carpeta);
  if (!d) return n;
  int desde = n;
  for (struct dirent *f; n < max && (f = readdir(d));) {
    size_t largo = strlen(f->d_name);
    if (largo < 5 || strcmp(f->d_name + largo - 4, ".png") != 0) continue;
    snprintf(rutas[n++], 512, "%s/%s", carpeta, f->d_name);
  }
  closedir(d);
  // Las rutas de una carpeta comparten el principio: ordenarlas es ordenar
  // los nombres. readdir no promete ningun orden.
  qsort(rutas[desde], (size_t)(n - desde), sizeof rutas[0], comparar_nombres);
  return n;
}

static int sprites(const char **rutas, int max) {
  static char guardadas[SPRITES_MAX][512];
  static int n = -1;
  if (n < 0) {
    n = juntar_pngs(RUTA_ASSETS "/sprites", guardadas, 0, SPRITES_MAX);
    n = juntar_pngs(RUTA_ASSETS "/tiles", guardadas, n, SPRITES_MAX);
  }
  int k = 0;
  for (; k < n && k < max; k++) rutas[k] = guardadas[k];
  return k;
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
  .escena = escena,
  .inspeccionar_objeto = inspeccionar_objeto,
  .mover_objeto = mover_objeto,
  .mundo_tamano = mundo_tamano,
  .dibujar_mundo = dibujar_mundo,
  .sprites = sprites,
};
