// ============================================================
// VimMon — EDITOR 2D
//
//   ┌──────────────────────────────────────────────────────────┐
//   │ VimMon   [Play] [Pausa] [Stop]                     60 fps │ barra
//   ├──────────┬────────────────────────────────┬──────────────┤
//   │ PROYECTO │                                │ INSPECTOR    │
//   │ Delirio  │        el juego (su lienzo)    │ jugador ...  │
//   │          │                                │              │
//   ├──────────┴────────────────────────────────┴──────────────┤
//   │ Delirio  corriendo  lienzo 640x360 x2                     │ estado
//   └──────────────────────────────────────────────────────────┘
//
// El editor dibuja a escala 1 (texto nitido). El juego dibuja en SU lienzo,
// chico, a su escala de pixel art, y el editor estira ese lienzo dentro del
// rectangulo del centro. El juego no sabe que hay un editor alrededor.
//
// Teclas: F5 Play/Stop, F6 Pausa. Esc con el juego corriendo lo para.
//
//   make editor && ./build/editor2d
//   ./build/editor2d --captura /tmp/e.bmp 3000   (Play, 3 s, guarda y sale)
// ============================================================

#include "editor2d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BARRA_ALTO     34
#define ESTADO_ALTO    24
#define PANEL_IZQ      200
#define PANEL_DER      260
#define MARGEN         10

#define C_FONDO     RGB2(0x14, 0x11, 0x19)
#define C_PANEL     RGB2(0x1e, 0x1a, 0x25)
#define C_BORDE     RGB2(0x2e, 0x22, 0x2f)
#define C_TEXTO     RGB2(0xc7, 0xc2, 0xb8)
#define C_TENUE     RGB2(0x6b, 0x63, 0x6f)
#define C_ACENTO    RGB2(0xfb, 0xb9, 0x54)
#define C_BOTON     RGB2(0x2b, 0x25, 0x33)
#define C_BOTON_ON  RGB2(0x45, 0x3a, 0x52)
#define C_VACIO     RGB2(0x0d, 0x0b, 0x12)

typedef enum { DETENIDO, CORRIENDO, PAUSADO } Modo;

typedef struct {
  const Renderer2D *r;
  Modulo2D *modulo;
  int cargado;
  Modo modo;
  Textura2D lienzo;            // donde dibuja el juego
  int lienzo_w, lienzo_h, escala;
  Rect2D vista;                // rectangulo del juego en la ventana
  float mx, my;
  int clic, clic_antes;        // flanco: solo el frame en que se aprieta
  int f5_antes, f6_antes, esc_antes;
  float fps;
} Editor;

static int dentro(Rect2D r, float x, float y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static void caja(const Renderer2D *r, Rect2D c, uint32_t fondo, uint32_t borde) {
  r->rect(c, borde);
  r->rect((Rect2D){ c.x + 1, c.y + 1, c.w - 2, c.h - 2 }, fondo);
}

// Un boton: devuelve 1 el frame en que se hace clic sobre el.
static int boton(Editor *ed, float x, float y, const char *txt, int activo) {
  const Renderer2D *r = ed->r;
  float w = r->texto_ancho(txt) + 20, h = BARRA_ALTO - 10;
  Rect2D b = { x, y, w, h };
  int encima = dentro(b, ed->mx, ed->my);
  caja(r, b, activo ? C_BOTON_ON : encima ? C_BOTON_ON : C_BOTON, activo ? C_ACENTO : C_BORDE);
  r->texto(x + 10, y + (h - (float)r->texto_alto()) / 2, txt, activo ? C_ACENTO : C_TEXTO);
  return encima && ed->clic && !ed->clic_antes;
}

static void titulo_panel(const Renderer2D *r, float x, float y, const char *t) {
  r->texto(x, y, t, C_TENUE);
}

// ── Ciclo de vida del juego ──────────────────────────────────

static void play(Editor *ed) {
  if (!ed->cargado) return;
  if (ed->modo == DETENIDO) ed->modulo->iniciar();
  ed->modo = CORRIENDO;
}

static void stop(Editor *ed) {
  if (ed->modo == DETENIDO) return;
  ed->modulo->terminar();
  ed->modo = DETENIDO;
}

static void pausa(Editor *ed) {
  if (ed->modo == CORRIENDO) ed->modo = PAUSADO;
  else if (ed->modo == PAUSADO) ed->modo = CORRIENDO;
}

static void abrir(Editor *ed, Modulo2D *m) {
  if (ed->modulo == m && ed->cargado) return;
  if (ed->modulo) {
    stop(ed);
    if (ed->cargado) ed->modulo->descargar(ed->r);
  }
  ed->modulo = m;
  ed->cargado = m->cargar(ed->r) == 0;
  if (!ed->cargado) fprintf(stderr, "[editor2d] no pude cargar %s\n", m->nombre);
}

// El lienzo del juego sigue al rectangulo del centro: misma regla de escala
// que el navegador (entera, la mas cercana a alto / alto_objetivo).
static void ajustar_lienzo(Editor *ed) {
  int vw = (int)ed->vista.w, vh = (int)ed->vista.h;
  int obj = ed->modulo ? ed->modulo->alto_objetivo : 480;
  int s = obj > 0 ? (int)((float)vh / (float)obj + 0.5f) : 1;
  if (s < 1) s = 1;
  int w = vw / s, h = vh / s;
  if (w < 1 || h < 1) return;
  if (ed->lienzo && w == ed->lienzo_w && h == ed->lienzo_h && s == ed->escala) return;
  ed->r->textura_liberar(ed->lienzo);
  ed->lienzo = ed->r->lienzo_crear(w, h);
  ed->lienzo_w = w;
  ed->lienzo_h = h;
  ed->escala = s;
}

// Donde cae el lienzo dentro de la vista (centrado).
static Rect2D lienzo_en_pantalla(const Editor *ed) {
  float w = (float)(ed->lienzo_w * ed->escala), h = (float)(ed->lienzo_h * ed->escala);
  return (Rect2D){ ed->vista.x + (ed->vista.w - w) / 2, ed->vista.y + (ed->vista.h - h) / 2, w, h };
}

// ── Un frame ─────────────────────────────────────────────────

static void entrada_del_juego(const Editor *ed, Entrada2D *e) {
  const Renderer2D *r = ed->r;
  Rect2D l = lienzo_en_pantalla(ed);
  memset(e, 0, sizeof *e);
  e->mouse_x = (ed->mx - l.x) / (float)ed->escala;
  e->mouse_y = (ed->my - l.y) / (float)ed->escala;
  e->mouse_dentro = r->mouse_dentro() && dentro(l, ed->mx, ed->my);
  for (int b = 0; b < M2_COUNT; b++) e->boton[b] = e->mouse_dentro && r->mouse_boton((Boton2D)b);
  // Las teclas del editor no le llegan al juego.
  for (int k = 0; k < T2_COUNT; k++)
    if (k != T2_F5 && k != T2_F6 && k != T2_ESCAPE) e->tecla[k] = r->tecla((Tecla2D)k);
}

static void dibujar_interfaz(Editor *ed) {
  const Renderer2D *r = ed->r;
  float W = (float)r->ancho_interno(), H = (float)r->alto_interno();
  char buf[128];

  // Barra
  r->rect((Rect2D){ 0, 0, W, BARRA_ALTO }, C_PANEL);
  r->rect((Rect2D){ 0, BARRA_ALTO - 1, W, 1 }, C_BORDE);
  r->texto(MARGEN, (BARRA_ALTO - (float)r->texto_alto()) / 2, "VimMon", C_ACENTO);
  float x = MARGEN + r->texto_ancho("VimMon") + 24;
  if (boton(ed, x, 5, "Play  F5", ed->modo == CORRIENDO)) play(ed);
  x += r->texto_ancho("Play  F5") + 28;
  if (boton(ed, x, 5, "Pausa  F6", ed->modo == PAUSADO)) pausa(ed);
  x += r->texto_ancho("Pausa  F6") + 28;
  if (boton(ed, x, 5, "Stop", 0)) stop(ed);
  snprintf(buf, sizeof buf, "%.0f fps", ed->fps);
  r->texto(W - MARGEN - r->texto_ancho(buf), (BARRA_ALTO - (float)r->texto_alto()) / 2, buf, C_TENUE);

  // Panel izquierdo: los juegos que hay
  float py = BARRA_ALTO, ph = H - BARRA_ALTO - ESTADO_ALTO;
  r->rect((Rect2D){ 0, py, PANEL_IZQ, ph }, C_PANEL);
  r->rect((Rect2D){ PANEL_IZQ - 1, py, 1, ph }, C_BORDE);
  titulo_panel(r, MARGEN, py + MARGEN, "PROYECTO");
  float ly = py + MARGEN + (float)r->texto_alto() + 8;
  for (int i = 0; i < n_modulos2d; i++) {
    Rect2D fila = { 4, ly - 3, PANEL_IZQ - 9, (float)r->texto_alto() + 6 };
    int elegido = modulos2d[i] == ed->modulo;
    if (elegido) r->rect(fila, C_BOTON_ON);
    r->texto(MARGEN + 4, ly, modulos2d[i]->nombre, elegido ? C_ACENTO : C_TEXTO);
    if (dentro(fila, ed->mx, ed->my) && ed->clic && !ed->clic_antes) abrir(ed, modulos2d[i]);
    ly += (float)r->texto_alto() + 8;
  }
  float ay = py + ph - 4 * ((float)r->texto_alto() + 4) - MARGEN;
  titulo_panel(r, MARGEN, ay, "TECLAS");
  r->texto(MARGEN, ay += (float)r->texto_alto() + 4, "F5   Play / Stop", C_TENUE);
  r->texto(MARGEN, ay += (float)r->texto_alto() + 4, "F6   Pausa", C_TENUE);
  r->texto(MARGEN, ay += (float)r->texto_alto() + 4, "Esc  para el juego", C_TENUE);

  // Panel derecho: lo que el juego cuenta de si mismo
  float dx = W - PANEL_DER;
  r->rect((Rect2D){ dx, py, PANEL_DER, ph }, C_PANEL);
  r->rect((Rect2D){ dx, py, 1, ph }, C_BORDE);
  titulo_panel(r, dx + MARGEN, py + MARGEN, "INSPECTOR");
  if (ed->modo != DETENIDO && ed->modulo->inspeccionar) {
    char lineas[INSPECTOR_LINEAS][INSPECTOR_LARGO];
    int n = ed->modulo->inspeccionar(lineas, INSPECTOR_LINEAS);
    for (int i = 0; i < n; i++)
      r->texto(dx + MARGEN, py + MARGEN + (float)(i + 1) * ((float)r->texto_alto() + 4) + 4,
               lineas[i], C_TEXTO);
  } else {
    r->texto(dx + MARGEN, py + MARGEN + (float)r->texto_alto() + 8, "(el juego no corre)", C_TENUE);
  }

  // Barra de estado
  float ey = H - ESTADO_ALTO;
  r->rect((Rect2D){ 0, ey, W, ESTADO_ALTO }, C_PANEL);
  r->rect((Rect2D){ 0, ey, W, 1 }, C_BORDE);
  static const char *MODOS[] = { "detenido", "corriendo", "en pausa" };
  snprintf(buf, sizeof buf, "%s   %s   lienzo %dx%d  x%d", ed->modulo ? ed->modulo->nombre : "-",
           ed->cargado ? MODOS[ed->modo] : "no se pudo cargar", ed->lienzo_w, ed->lienzo_h, ed->escala);
  r->texto(MARGEN, ey + (ESTADO_ALTO - (float)r->texto_alto()) / 2, buf, C_TENUE);
}

static void dibujar_juego(Editor *ed) {
  const Renderer2D *r = ed->r;
  Rect2D l = lienzo_en_pantalla(ed);
  r->rect(ed->vista, C_VACIO);
  if (ed->modo != DETENIDO && ed->lienzo) {
    r->sprite(ed->lienzo, (Rect2D){ 0, 0, (float)ed->lienzo_w, (float)ed->lienzo_h }, l, BLANCO2, 0);
  } else {
    const char *t = ed->cargado ? "Play (F5) para correr el juego" : "No se pudo cargar el juego";
    r->texto(l.x + (l.w - r->texto_ancho(t)) / 2, l.y + l.h / 2 - (float)r->texto_alto() / 2, t, C_TENUE);
  }
  // El marco del juego: acento mientras corre.
  uint32_t borde = ed->modo == CORRIENDO ? C_ACENTO : C_BORDE;
  r->rect((Rect2D){ l.x - 1, l.y - 1, l.w + 2, 1 }, borde);
  r->rect((Rect2D){ l.x - 1, l.y + l.h, l.w + 2, 1 }, borde);
  r->rect((Rect2D){ l.x - 1, l.y, 1, l.h }, borde);
  r->rect((Rect2D){ l.x + l.w, l.y, 1, l.h }, borde);
}

int main(int argc, char **argv) {
  const char *captura = NULL;
  int captura_ms = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--captura") == 0 && i + 1 < argc) {
      captura = argv[++i];
      if (i + 1 < argc && argv[i + 1][0] != '-') captura_ms = atoi(argv[++i]);
    }
  }

  Editor ed = { .r = &gpu2d_sdl_renderer2d, .escala = 1 };
  const Renderer2D *r = ed.r;
  if (r->init("VimMon - Editor 2D", 1440, 860, 0) != 0) return 1;
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
    ed.vista = (Rect2D){ PANEL_IZQ + MARGEN, BARRA_ALTO + MARGEN,
                         W - PANEL_IZQ - PANEL_DER - 2 * MARGEN, H - BARRA_ALTO - ESTADO_ALTO - 2 * MARGEN };
    r->mouse_pos(&ed.mx, &ed.my);
    ed.clic_antes = ed.clic;
    ed.clic = r->mouse_boton(M2_IZQ);

    int f5 = r->tecla(T2_F5), f6 = r->tecla(T2_F6), esc = r->tecla(T2_ESCAPE);
    if (f5 && !ed.f5_antes) {
      if (ed.modo == DETENIDO) play(&ed);
      else stop(&ed);
    }
    if (f6 && !ed.f6_antes) pausa(&ed);
    if (esc && !ed.esc_antes) stop(&ed);
    ed.f5_antes = f5;
    ed.f6_antes = f6;
    ed.esc_antes = esc;

    if (ed.cargado) ajustar_lienzo(&ed);
    if (ed.modo == CORRIENDO) {
      Entrada2D e;
      entrada_del_juego(&ed, &e);
      if (captura) e.mouse_dentro = 0;
      ed.modulo->actualizar(&e, dt, ed.lienzo_w, ed.lienzo_h);
    }

    r->frame_begin(C_FONDO);
    if (ed.modo != DETENIDO && ed.lienzo) {
      r->destino(ed.lienzo, RGB2(0, 0, 0));
      ed.modulo->dibujar(r, ed.lienzo_w, ed.lienzo_h);
      r->destino(TEXTURA2D_NULA, C_FONDO);
    }
    dibujar_juego(&ed);
    dibujar_interfaz(&ed);
    r->frame_end();

    if (captura && (simulado += 16) >= captura_ms) {
      int fallo = gpu2d_sdl_capturar_bmp(captura);
      printf("ventana %dx%d, juego %dx%d x%d -> %s\n", (int)W, (int)H, ed.lienzo_w, ed.lienzo_h,
             ed.escala, fallo ? "fallo" : captura);
      break;
    }
    if (!captura) r->delay_ms(1);
  }
  if (ed.modulo && ed.cargado) {
    stop(&ed);
    ed.modulo->descargar(r);
  }
  r->textura_liberar(ed.lienzo);
  r->shutdown();
  return 0;
}
