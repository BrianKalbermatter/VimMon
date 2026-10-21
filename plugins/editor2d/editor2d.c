// ============================================================
// VimMon — EDITOR 2D
//
//   ┌──────────────────────────────────────────────────────────┐
//   │ VimMon   [Play] [Pausa] [Stop]                     60 fps │ barra
//   ├──────────┬────────────────────────────────┬──────────────┤
//   │ PROYECTO │ [Juego] [Escena] [Sprites]     │ INSPECTOR    │
//   │ Delirio  │                                │ lo elegido   │
//   │──────────│      la pestana activa         │              │
//   │HIERARCHY │                                │              │
//   │ v Lab... │                                │              │
//   ├──────────┴────────────────────────────────┴──────────────┤
//   │ Delirio  corriendo  lienzo 640x360 x2                     │ estado
//   └──────────────────────────────────────────────────────────┘
//
// Las tres pestanas, como en Unity:
//   Juego    lo que ve el jugador, con la camara del juego y su escala
//            de pixel art
//   Escena   el mismo mundo con una camara libre, las cajas de los
//            actores, seleccionar y mover (vista_escena.c)
//   Sprites  la pizarra infinita para pintar los PNG del juego
//            (pizarra_sprites.c)
// Hierarchy e Inspector comparten la seleccion de la Escena.
//
// El editor dibuja a escala 1 (texto nitido). Cada pestana dibuja en SU
// lienzo y el editor lo pone en el centro: la pestana no sabe que hay un
// editor alrededor.
//
// Teclas: F5 Play/Stop, F6 Pausa, Esc para el juego, Ctrl+1/2/3 pestanas.
//
//   make editor && ./build/editor2d
//   ./build/editor2d --captura /tmp/e.bmp 3000 [--pestana escena|sprites]
// ============================================================

#include "editor2d.h"
#include "pizarra_sprites.h"
#include "ui2d.h"
#include "vista_escena.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BARRA_ALTO     34
#define PESTANAS_ALTO  28
#define ESTADO_ALTO    24
#define PANEL_IZQ      240
#define PANEL_DER      280
#define MARGEN         10

typedef enum { DETENIDO, CORRIENDO, PAUSADO } Modo;
typedef enum { PEST_JUEGO, PEST_ESCENA, PEST_SPRITES, PESTANAS } Pestana;
static const char *NOMBRE_PESTANA[PESTANAS] = { "Juego", "Escena", "Sprites" };

typedef struct {
  const Renderer2D *r;
  Modulo2D *modulo;
  int cargado;
  Modo modo;
  Pestana pestana;

  Textura2D lienzo;            // donde dibuja el juego (chico, pixel art)
  int lienzo_w, lienzo_h, escala;
  Textura2D lienzo_vista;      // Escena y Sprites (escala 1)
  int vista_w, vista_h;
  Rect2D centro;               // el rectangulo de la pestana en la ventana

  EntradaUI in;                // en px de la ventana
  int botones_antes[M2_COUNT];
  int teclas_antes[T2_COUNT];
  float fps;

  VistaEscena escena;
  Pizarra pizarra;
} Editor;

static int dentro(Rect2D r, float x, float y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static void caja(const Renderer2D *r, Rect2D c, uint32_t fondo, uint32_t borde) {
  r->rect(c, borde);
  r->rect((Rect2D){ c.x + 1, c.y + 1, c.w - 2, c.h - 2 }, fondo);
}

// Un boton: devuelve 1 el frame en que se hace clic sobre el.
static int boton(Editor *ed, float x, float y, float h, const char *txt, int activo) {
  const Renderer2D *r = ed->r;
  float w = r->texto_ancho(txt) + 20;
  Rect2D b = { x, y, w, h };
  int encima = dentro(b, ed->in.mx, ed->in.my);
  caja(r, b, activo || encima ? UI_BOTON_ON : UI_BOTON, activo ? UI_ACENTO : UI_BORDE);
  r->texto(x + 10, y + (h - (float)r->texto_alto()) / 2, txt, activo ? UI_ACENTO : UI_TEXTO);
  return encima && ed->in.apreto[M2_IZQ];
}

// ── Ciclo de vida del juego ──────────────────────────────────

static void play(Editor *ed) {
  if (!ed->cargado) return;
  if (ed->modo == DETENIDO) {
    ed->modulo->iniciar();
    ed->escena.encuadrado = 0;   // partida nueva: la Escena vuelve a centrarse
  }
  ed->modo = CORRIENDO;
}

static void stop(Editor *ed) {
  if (ed->modo == DETENIDO) return;
  ed->modulo->terminar();
  ed->modo = DETENIDO;
  ed->escena.n_objs = 0;
  ed->escena.seleccion = -1;
}

static void pausa(Editor *ed) {
  if (ed->modo == CORRIENDO) ed->modo = PAUSADO;
  else if (ed->modo == PAUSADO) ed->modo = CORRIENDO;
}

static void abrir(Editor *ed, Modulo2D *m) {
  if (ed->modulo == m && ed->cargado) return;
  if (ed->modulo) {
    stop(ed);
    pizarra_cerrar(&ed->pizarra, ed->r);
    if (ed->cargado) ed->modulo->descargar(ed->r);
  }
  ed->modulo = m;
  ed->cargado = m->cargar(ed->r) == 0;
  if (!ed->cargado) {
    fprintf(stderr, "[editor2d] no pude cargar %s\n", m->nombre);
    return;
  }
  escena_iniciar(&ed->escena);
  if (m->sprites) {
    const char *rutas[SPRITES_MAX];
    int n = m->sprites(rutas, SPRITES_MAX);
    pizarra_abrir(&ed->pizarra, ed->r, rutas, n);
  }
}

// ── Lienzos ──────────────────────────────────────────────────

// El lienzo del juego sigue al rectangulo del centro: misma regla de escala
// que el navegador (entera, la mas cercana a alto / alto_objetivo).
static void ajustar_lienzos(Editor *ed) {
  int vw = (int)ed->centro.w, vh = (int)ed->centro.h;
  if (vw < 1 || vh < 1) return;
  int obj = ed->modulo ? ed->modulo->alto_objetivo : 480;
  int s = obj > 0 ? (int)((float)vh / (float)obj + 0.5f) : 1;
  if (s < 1) s = 1;
  int w = vw / s, h = vh / s;
  if (!ed->lienzo || w != ed->lienzo_w || h != ed->lienzo_h || s != ed->escala) {
    ed->r->textura_liberar(ed->lienzo);
    ed->lienzo = ed->r->lienzo_crear(w, h);
    ed->lienzo_w = w;
    ed->lienzo_h = h;
    ed->escala = s;
  }
  if (!ed->lienzo_vista || vw != ed->vista_w || vh != ed->vista_h) {
    ed->r->textura_liberar(ed->lienzo_vista);
    ed->lienzo_vista = ed->r->lienzo_crear(vw, vh);
    ed->vista_w = vw;
    ed->vista_h = vh;
  }
}

// Donde cae el lienzo del juego dentro del centro (centrado).
static Rect2D juego_en_pantalla(const Editor *ed) {
  float w = (float)(ed->lienzo_w * ed->escala), h = (float)(ed->lienzo_h * ed->escala);
  return (Rect2D){ ed->centro.x + (ed->centro.w - w) / 2, ed->centro.y + (ed->centro.h - h) / 2, w, h };
}

// ── Entrada ──────────────────────────────────────────────────

static void leer_entrada(Editor *ed) {
  const Renderer2D *r = ed->r;
  EntradaUI *in = &ed->in;
  r->mouse_pos(&in->mx, &in->my);
  in->dentro = r->mouse_dentro();
  in->rueda = r->rueda();
  for (int b = 0; b < M2_COUNT; b++) {
    in->boton[b] = r->mouse_boton((Boton2D)b);
    in->apreto[b] = in->boton[b] && !ed->botones_antes[b];
    in->solto[b] = !in->boton[b] && ed->botones_antes[b];
    ed->botones_antes[b] = in->boton[b];
  }
  for (int k = 0; k < T2_COUNT; k++) {
    in->tecla[k] = r->tecla((Tecla2D)k);
    in->tecla_apreto[k] = in->tecla[k] && !ed->teclas_antes[k];
    ed->teclas_antes[k] = in->tecla[k];
  }
}

// La misma entrada vista desde el lienzo de una pestana (escala 1).
static EntradaUI entrada_en_vista(const Editor *ed) {
  EntradaUI e = ed->in;
  e.mx -= ed->centro.x;
  e.my -= ed->centro.y;
  e.dentro = ed->in.dentro && dentro(ed->centro, ed->in.mx, ed->in.my);
  if (!e.dentro) {
    // Fuera de la vista no se empieza nada nuevo, pero un arrastre que ya
    // venia sigue y se puede soltar.
    memset(e.apreto, 0, sizeof e.apreto);
    e.rueda = 0;
  }
  return e;
}

static void entrada_del_juego(const Editor *ed, Entrada2D *e) {
  Rect2D l = juego_en_pantalla(ed);
  memset(e, 0, sizeof *e);
  e->mouse_x = (ed->in.mx - l.x) / (float)ed->escala;
  e->mouse_y = (ed->in.my - l.y) / (float)ed->escala;
  // Solo la pestana Juego le pasa el jugador al juego: en Escena y Sprites
  // el mouse y el teclado son del editor.
  if (ed->pestana != PEST_JUEGO) return;
  e->mouse_dentro = ed->in.dentro && dentro(l, ed->in.mx, ed->in.my);
  for (int b = 0; b < M2_COUNT; b++) e->boton[b] = e->mouse_dentro && ed->in.boton[b];
  for (int k = 0; k < T2_COUNT; k++)
    if (k != T2_F5 && k != T2_F6 && k != T2_ESCAPE) e->tecla[k] = ed->in.tecla[k];
}

// ── Interfaz ─────────────────────────────────────────────────

static void dibujar_barra(Editor *ed, float W) {
  const Renderer2D *r = ed->r;
  char buf[64];
  r->rect((Rect2D){ 0, 0, W, BARRA_ALTO }, UI_PANEL);
  r->rect((Rect2D){ 0, BARRA_ALTO - 1, W, 1 }, UI_BORDE);
  r->texto(MARGEN, (BARRA_ALTO - (float)r->texto_alto()) / 2, "VimMon", UI_ACENTO);
  float x = MARGEN + r->texto_ancho("VimMon") + 24, h = BARRA_ALTO - 10;
  if (boton(ed, x, 5, h, "Play  F5", ed->modo == CORRIENDO)) play(ed);
  x += r->texto_ancho("Play  F5") + 28;
  if (boton(ed, x, 5, h, "Pausa  F6", ed->modo == PAUSADO)) pausa(ed);
  x += r->texto_ancho("Pausa  F6") + 28;
  if (boton(ed, x, 5, h, "Stop", 0)) stop(ed);
  snprintf(buf, sizeof buf, "%.0f fps", ed->fps);
  r->texto(W - MARGEN - r->texto_ancho(buf), (BARRA_ALTO - (float)r->texto_alto()) / 2, buf, UI_TENUE);
}

static void dibujar_pestanas(Editor *ed) {
  float x = ed->centro.x, y = ed->centro.y - PESTANAS_ALTO;
  for (int i = 0; i < PESTANAS; i++) {
    if (boton(ed, x, y + 2, PESTANAS_ALTO - 6, NOMBRE_PESTANA[i], ed->pestana == (Pestana)i))
      ed->pestana = (Pestana)i;
    x += ed->r->texto_ancho(NOMBRE_PESTANA[i]) + 26;
  }
}

static void dibujar_panel_izq(Editor *ed, float py, float ph) {
  const Renderer2D *r = ed->r;
  float th = (float)r->texto_alto();
  r->rect((Rect2D){ 0, py, PANEL_IZQ, ph }, UI_PANEL);
  r->rect((Rect2D){ PANEL_IZQ - 1, py, 1, ph }, UI_BORDE);

  r->texto(MARGEN, py + MARGEN, "PROYECTO", UI_TENUE);
  float ly = py + MARGEN + th + 8;
  for (int i = 0; i < n_modulos2d; i++) {
    Rect2D fila = { 4, ly - 3, PANEL_IZQ - 9, th + 6 };
    int elegido = modulos2d[i] == ed->modulo;
    if (elegido) r->rect(fila, UI_BOTON_ON);
    r->texto(MARGEN + 4, ly, modulos2d[i]->nombre, elegido ? UI_ACENTO : UI_TEXTO);
    if (dentro(fila, ed->in.mx, ed->in.my) && ed->in.apreto[M2_IZQ]) abrir(ed, modulos2d[i]);
    ly += th + 8;
  }

  float hy = ly + 8;
  r->rect((Rect2D){ 0, hy, PANEL_IZQ - 1, 1 }, UI_BORDE);
  r->texto(MARGEN, hy + MARGEN, "HIERARCHY", UI_TENUE);
  Rect2D zona = { 0, hy + MARGEN + th + 6, PANEL_IZQ - 1, py + ph - (hy + MARGEN + th + 6) };
  jerarquia_actualizar(&ed->escena, &ed->in, zona, r->texto_alto());
  jerarquia_dibujar(&ed->escena, r, zona);
}

static void dibujar_inspector(Editor *ed, float W, float py, float ph) {
  const Renderer2D *r = ed->r;
  float dx = W - PANEL_DER, th = (float)r->texto_alto();
  r->rect((Rect2D){ dx, py, PANEL_DER, ph }, UI_PANEL);
  r->rect((Rect2D){ dx, py, 1, ph }, UI_BORDE);
  r->texto(dx + MARGEN, py + MARGEN, "INSPECTOR", UI_TENUE);

  // Que muestra: en Sprites, el sprite elegido; si hay un objeto elegido en
  // la escena, ese objeto; si no, el estado general del juego.
  char lineas[INSPECTOR_LINEAS][INSPECTOR_LARGO];
  int n = 0;
  const char *titulo = NULL;
  const VistaEscena *v = &ed->escena;
  if (ed->pestana == PEST_SPRITES) {
    n = pizarra_inspeccionar(&ed->pizarra, lineas, INSPECTOR_LINEAS);
  } else if (ed->modo != DETENIDO && v->seleccion >= 0 && v->seleccion < v->n_objs &&
             ed->modulo->inspeccionar_objeto) {
    titulo = v->objs[v->seleccion].nombre;
    n = ed->modulo->inspeccionar_objeto(v->seleccion, lineas, INSPECTOR_LINEAS);
  } else if (ed->modo != DETENIDO && ed->modulo->inspeccionar) {
    titulo = ed->modulo->nombre;
    n = ed->modulo->inspeccionar(lineas, INSPECTOR_LINEAS);
  }
  float y = py + MARGEN + th + 8;
  if (titulo) {
    r->texto(dx + MARGEN, y, titulo, UI_ACENTO);
    y += th + 8;
  }
  if (n == 0) r->texto(dx + MARGEN, y, "(el juego no corre)", UI_TENUE);
  for (int i = 0; i < n; i++, y += th + 4) r->texto(dx + MARGEN, y, lineas[i], UI_TEXTO);
}

static void dibujar_estado(Editor *ed, float W, float H) {
  const Renderer2D *r = ed->r;
  char buf[192];
  float ey = H - ESTADO_ALTO;
  r->rect((Rect2D){ 0, ey, W, ESTADO_ALTO }, UI_PANEL);
  r->rect((Rect2D){ 0, ey, W, 1 }, UI_BORDE);
  static const char *MODOS[] = { "detenido", "corriendo", "en pausa" };
  snprintf(buf, sizeof buf, "%s   %s   juego %dx%d x%d      F5 Play/Stop   F6 Pausa   Esc parar   Ctrl+1/2/3 pestanas",
           ed->modulo ? ed->modulo->nombre : "-", ed->cargado ? MODOS[ed->modo] : "no se pudo cargar",
           ed->lienzo_w, ed->lienzo_h, ed->escala);
  r->texto(MARGEN, ey + (ESTADO_ALTO - (float)r->texto_alto()) / 2, buf, UI_TENUE);
}

static void marco(const Renderer2D *r, Rect2D l, uint32_t borde) {
  r->rect((Rect2D){ l.x - 1, l.y - 1, l.w + 2, 1 }, borde);
  r->rect((Rect2D){ l.x - 1, l.y + l.h, l.w + 2, 1 }, borde);
  r->rect((Rect2D){ l.x - 1, l.y, 1, l.h }, borde);
  r->rect((Rect2D){ l.x + l.w, l.y, 1, l.h }, borde);
}

static void dibujar_centro(Editor *ed) {
  const Renderer2D *r = ed->r;
  r->rect(ed->centro, UI_VACIO);
  if (ed->pestana == PEST_JUEGO) {
    Rect2D l = juego_en_pantalla(ed);
    if (ed->modo != DETENIDO && ed->lienzo) {
      r->sprite(ed->lienzo, (Rect2D){ 0, 0, (float)ed->lienzo_w, (float)ed->lienzo_h }, l, BLANCO2, 0);
    } else {
      const char *t = ed->cargado ? "Play (F5) para correr el juego" : "No se pudo cargar el juego";
      r->texto(l.x + (l.w - r->texto_ancho(t)) / 2, l.y + l.h / 2 - (float)r->texto_alto() / 2, t, UI_TENUE);
    }
    marco(r, l, ed->modo == CORRIENDO ? UI_ACENTO : UI_BORDE);
    return;
  }
  // Escena sin partida: no hay mundo que mostrar (el laberinto se genera
  // al darle Play).
  if (ed->pestana == PEST_ESCENA && ed->modo == DETENIDO) {
    const char *t = "Play (F5) para ver la escena";
    r->texto(ed->centro.x + (ed->centro.w - r->texto_ancho(t)) / 2,
             ed->centro.y + ed->centro.h / 2, t, UI_TENUE);
  } else if (ed->lienzo_vista) {
    r->sprite(ed->lienzo_vista, (Rect2D){ 0, 0, (float)ed->vista_w, (float)ed->vista_h }, ed->centro,
              BLANCO2, 0);
  }
  marco(r, ed->centro, UI_BORDE);
}

// ── Programa ─────────────────────────────────────────────────

int main(int argc, char **argv) {
  const char *captura = NULL;
  int captura_ms = 0;
  Pestana pestana_inicial = PEST_JUEGO;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) {
      captura = argv[++i];
      if (i + 1 < argc && argv[i + 1][0] != '-') captura_ms = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--pestana") == 0 && i + 1 < argc) {
      i++;
      if (strcmp(argv[i], "escena") == 0) pestana_inicial = PEST_ESCENA;
      else if (strcmp(argv[i], "sprites") == 0) pestana_inicial = PEST_SPRITES;
    }
  }

  static Editor ed;   // static: la pizarra y la escena son grandes para la pila
  ed.r = &gpu2d_sdl_renderer2d;
  ed.escala = 1;
  ed.pestana = pestana_inicial;
  const Renderer2D *r = ed.r;
  if (r->init("VimMon - Editor 2D", 1440, 860, 0) != 0) return 1;
  escena_iniciar(&ed.escena);
  if (n_modulos2d > 0) abrir(&ed, modulos2d[0]);
  if (captura) play(&ed);

  uint32_t antes = r->ticks_ms();
  int simulado = 0;
  while (!r->poll_quit()) {
    float dt;
    if (captura) {
      dt = 16;
    } else {
      uint32_t ahora = r->ticks_ms();
      dt = (float)(ahora - antes);
      if (dt > 100) dt = 100;
      antes = ahora;
    }
    if (dt > 0) ed.fps += (1000.0f / dt - ed.fps) * 0.05f;

    float W = (float)r->ancho_interno(), H = (float)r->alto_interno();
    float py = BARRA_ALTO, ph = H - BARRA_ALTO - ESTADO_ALTO;
    ed.centro = (Rect2D){ PANEL_IZQ + MARGEN, BARRA_ALTO + PESTANAS_ALTO + 4,
                          W - PANEL_IZQ - PANEL_DER - 2 * MARGEN,
                          H - BARRA_ALTO - PESTANAS_ALTO - 4 - ESTADO_ALTO - MARGEN };
    leer_entrada(&ed);
    if (captura) memset(&ed.in, 0, sizeof ed.in);

    const int *ta = ed.in.tecla_apreto;
    if (ta[T2_F5]) {
      if (ed.modo == DETENIDO) play(&ed);
      else stop(&ed);
    }
    if (ta[T2_F6]) pausa(&ed);
    if (ta[T2_ESCAPE]) stop(&ed);
    if (ed.in.tecla[T2_CTRL]) {
      if (ta[T2_1]) ed.pestana = PEST_JUEGO;
      if (ta[T2_2]) ed.pestana = PEST_ESCENA;
      if (ta[T2_3]) ed.pestana = PEST_SPRITES;
    }

    if (ed.cargado) ajustar_lienzos(&ed);
    // El juego corre en cualquier pestana, como en Unity: mirar la Escena
    // no lo frena. Solo la pestana Juego le pasa el mouse y el teclado.
    if (ed.modo == CORRIENDO) {
      Entrada2D e;
      entrada_del_juego(&ed, &e);
      ed.modulo->actualizar(&e, dt, ed.lienzo_w, ed.lienzo_h);
    }
    if (ed.modo != DETENIDO) escena_refrescar(&ed.escena, ed.modulo);

    EntradaUI ev = entrada_en_vista(&ed);
    if (ed.pestana == PEST_ESCENA && ed.modo != DETENIDO)
      escena_actualizar(&ed.escena, ed.modulo, &ev, ed.vista_w, ed.vista_h);
    if (ed.pestana == PEST_SPRITES)
      pizarra_actualizar(&ed.pizarra, r, &ev, ed.vista_w, ed.vista_h, dt);

    r->frame_begin(UI_FONDO);
    if (ed.modo != DETENIDO && ed.lienzo && ed.pestana == PEST_JUEGO) {
      r->destino(ed.lienzo, RGB2(0, 0, 0));
      ed.modulo->dibujar(r, ed.lienzo_w, ed.lienzo_h);
    }
    if (ed.lienzo_vista && ed.pestana == PEST_ESCENA && ed.modo != DETENIDO) {
      r->destino(ed.lienzo_vista, UI_VACIO);
      escena_dibujar(&ed.escena, ed.modulo, r, ed.vista_w, ed.vista_h);
    }
    if (ed.lienzo_vista && ed.pestana == PEST_SPRITES) {
      r->destino(ed.lienzo_vista, UI_VACIO);
      pizarra_dibujar(&ed.pizarra, r, ed.vista_w, ed.vista_h);
    }
    r->destino(TEXTURA2D_NULA, UI_FONDO);

    dibujar_centro(&ed);
    dibujar_barra(&ed, W);
    dibujar_pestanas(&ed);
    dibujar_panel_izq(&ed, py, ph);
    dibujar_inspector(&ed, W, py, ph);
    dibujar_estado(&ed, W, H);
    r->frame_end();

    if (captura && (simulado += 16) >= captura_ms) {
      int fallo = gpu2d_sdl_capturar_bmp(captura);
      printf("ventana %dx%d, juego %dx%d x%d, %d objetos -> %s\n", (int)W, (int)H, ed.lienzo_w,
             ed.lienzo_h, ed.escala, ed.escena.n_objs, fallo ? "fallo" : captura);
      break;
    }
    if (!captura) r->delay_ms(1);
  }
  if (ed.modulo && ed.cargado) {
    stop(&ed);
    pizarra_cerrar(&ed.pizarra, r);
    ed.modulo->descargar(r);
  }
  r->textura_liberar(ed.lienzo);
  r->textura_liberar(ed.lienzo_vista);
  r->shutdown();
  return 0;
}
