// ============================================================
// VimMon — PIZARRA INFINITA DE SPRITES
//
//   ┌──────────────────────────────────────────────────────────┐
//   │ [Lapiz B] [Goma E] [Gotero I]  ■  ■■■■■■■■■■■■ paleta     │ barra
//   │                                                          │
//   │   personaje_3.png          maze_tiles.png                │
//   │   ┌────────────┐           ┌──────────────────┐          │
//   │   │  (sprite)  │           │                  │          │
//   │   └────────────┘           └──────────────────┘          │
//   │ personaje_3.png  1024x3648  zoom 8.00x  * sin guardar     │ estado
//   └──────────────────────────────────────────────────────────┘
//
// Dos pasadas al dibujar: el MUNDO (damero, sprites, borde y grilla) con
// camara y zoom de la pizarra, y la PANTALLA (titulos, barra, estado) a
// escala 1, para que el texto quede nitido a cualquier zoom.
//
// Se pinta en la copia de RAM del nodo (NodoSprite.pix) y la textura se
// sube entera con textura_actualizar, una sola vez por frame.
// ============================================================

#include "pizarra_sprites.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEPARACION    48       // entre nodos, px de pizarra
#define ANCHO_FILA    2048     // al pasarse, el nodo siguiente baja de fila
#define ZOOM_PASO     1.15f    // por cada muesca de la rueda
#define ZOOM_MIN      0.1f
#define ZOOM_MAX      64.0f
#define ZOOM_GRILLA   8.0f     // desde aca se ve la grilla de pixeles
#define MAX_GRILLA    20000    // cuadrilateros de grilla por frame, como mucho
#define BARRA_ALTO    30
#define ESTADO_ALTO   22
#define MARGEN        8
#define MUESTRA       18       // lado de un cuadradito de la paleta
#define PALETA_MAX    34       // 32 del sprite + negro y blanco
#define MENSAJE_MS    2500.0f

#define DAMERO_A      RGB2(0x1c, 0x1a, 0x22)
#define DAMERO_B      RGB2(0x26, 0x23, 0x2d)
#define GRILLA        ARGB(0x50, 0x00, 0x00, 0x00)
#define NEGRO2        0xFF000000u

static const char *const NOMBRES_HERR[] = { "lapiz", "goma", "gotero" };
static const char *const BOTONES_HERR[] = { "Lapiz B", "Goma E", "Gotero I" };

// La paleta del sprite seleccionado. Contar los colores de un sprite grande
// (millones de pixeles) cuesta, asi que se calcula solo cuando cambia la
// seleccion o termina un trazo, y queda guardada aca: el struct Pizarra es
// del header y no tiene lugar para ella. Hay una sola pizarra por editor.
static struct {
  int valida;
  uint32_t col[PALETA_MAX];
  int n;
} g_paleta;

static int dentro(Rect2D r, float x, float y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static void caja(const Renderer2D *r, Rect2D c, uint32_t fondo, uint32_t borde) {
  r->rect(c, borde);
  r->rect((Rect2D){ c.x + 1, c.y + 1, c.w - 2, c.h - 2 }, fondo);
}

// ── Camara ───────────────────────────────────────────────────

// cam_x, cam_y es el CENTRO de la vista; el renderer quiere la esquina.
static float zoom_de(const Pizarra *p) { return p->zoom > 0 ? p->zoom : 1; }

static void a_mundo(const Pizarra *p, float sx, float sy, int w, int h, float *wx, float *wy) {
  float z = zoom_de(p);
  *wx = p->cam_x + (sx - (float)w / 2) / z;
  *wy = p->cam_y + (sy - (float)h / 2) / z;
}

static void a_pantalla(const Pizarra *p, float wx, float wy, int w, int h, float *sx, float *sy) {
  float z = zoom_de(p);
  *sx = (wx - p->cam_x) * z + (float)w / 2;
  *sy = (wy - p->cam_y) * z + (float)h / 2;
}

// Encuadra un nodo: entra entero en la vista, y si se puede agrandar se
// usa un zoom entero (cada pixel del arte, un cuadrado parejo).
static void encuadrar(Pizarra *p, int i, int w, int h) {
  const NodoSprite *n = &p->nodos[i];
  float util_h = (float)(h - BARRA_ALTO - ESTADO_ALTO);
  float z = fminf((float)w * 0.8f / (float)n->w, util_h * 0.8f / (float)n->h);
  if (z >= 1) z = floorf(z);
  p->zoom = fmaxf(ZOOM_MIN, fminf(ZOOM_MAX, z));
  p->cam_x = n->x + (float)n->w / 2;
  p->cam_y = n->y + (float)n->h / 2 - (float)(BARRA_ALTO - ESTADO_ALTO) / 2 / p->zoom;
}

// ── Paleta ───────────────────────────────────────────────────

static float luminancia(uint32_t c) {
  return 0.299f * (float)((c >> 16) & 0xFF) + 0.587f * (float)((c >> 8) & 0xFF) + 0.114f * (float)(c & 0xFF);
}

static int por_luminancia(const void *a, const void *b) {
  float la = luminancia(*(const uint32_t *)a), lb = luminancia(*(const uint32_t *)b);
  return (la > lb) - (la < lb);
}

// Los colores opacos que mas se usan en el sprite (hasta 32), de oscuro a
// claro, con negro y blanco siempre al principio. Tabla hash chica: si el
// sprite tiene mas colores que lugares, los que no entran no se cuentan.
static void calcular_paleta(const Pizarra *p) {
  enum { HUECOS = 4096 };
  static uint32_t color[HUECOS];
  static int cuenta[HUECOS];
  g_paleta.valida = 1;
  g_paleta.n = 0;
  g_paleta.col[g_paleta.n++] = NEGRO2;
  g_paleta.col[g_paleta.n++] = BLANCO2;
  if (p->seleccion < 0) return;

  const NodoSprite *n = &p->nodos[p->seleccion];
  memset(cuenta, 0, sizeof cuenta);
  size_t total = (size_t)n->w * (size_t)n->h;
  for (size_t k = 0; k < total; k++) {
    uint32_t c = n->pix[k];
    if ((c >> 24) != 0xFF || c == NEGRO2 || c == BLANCO2) continue;
    uint32_t h = (c * 2654435761u) % HUECOS;
    for (int intento = 0; intento < 16; intento++, h = (h + 1) % HUECOS) {
      if (cuenta[h] == 0) { color[h] = c; cuenta[h] = 1; break; }
      if (color[h] == c) { cuenta[h]++; break; }
    }
  }
  // Los mas usados: seleccion simple, la paleta es chica.
  uint32_t elegidos[PALETA_MAX - 2];
  int n_elegidos = 0;
  while (n_elegidos < PALETA_MAX - 2) {
    int mejor = -1;
    for (int k = 0; k < HUECOS; k++)
      if (cuenta[k] > 0 && (mejor < 0 || cuenta[k] > cuenta[mejor])) mejor = k;
    if (mejor < 0) break;
    elegidos[n_elegidos++] = color[mejor];
    cuenta[mejor] = 0;
  }
  qsort(elegidos, (size_t)n_elegidos, sizeof elegidos[0], por_luminancia);
  memcpy(g_paleta.col + g_paleta.n, elegidos, (size_t)n_elegidos * sizeof elegidos[0]);
  g_paleta.n += n_elegidos;
}

// ── Barra de herramientas ────────────────────────────────────

// Donde cae cada cosa de la barra. La usan actualizar (para saber que se
// toco) y dibujar (para pintarla): asi nunca se desfasan.
typedef struct {
  Rect2D fondo;
  Rect2D herr[3];
  Rect2D actual;               // el color que pinta
  Rect2D muestra[PALETA_MAX];
  int n_muestras;
} Barra;

static Barra armar_barra(const Renderer2D *r, int w) {
  Barra b;
  b.fondo = (Rect2D){ 0, 0, (float)w, BARRA_ALTO };
  float x = MARGEN, alto = BARRA_ALTO - 8;
  for (int i = 0; i < 3; i++) {
    b.herr[i] = (Rect2D){ x, 4, r->texto_ancho(BOTONES_HERR[i]) + 16, alto };
    x += b.herr[i].w + 6;
  }
  x += 10;
  b.actual = (Rect2D){ x, 4, alto, alto };
  x += alto + 14;
  b.n_muestras = 0;
  for (int i = 0; i < g_paleta.n && x + MUESTRA <= (float)w - MARGEN; i++) {
    b.muestra[b.n_muestras++] = (Rect2D){ x, (BARRA_ALTO - MUESTRA) / 2.0f, MUESTRA, MUESTRA };
    x += MUESTRA + 2;
  }
  return b;
}

// ── Nodos ────────────────────────────────────────────────────

// Rectangulo del titulo en pantalla: arriba del nodo, a escala 1.
static Rect2D titulo_en_pantalla(const Pizarra *p, const Renderer2D *r, int i, int w, int h) {
  const NodoSprite *n = &p->nodos[i];
  float sx, sy, th = (float)r->texto_alto();
  a_pantalla(p, n->x, n->y, w, h, &sx, &sy);
  return (Rect2D){ sx, sy - th - 4, r->texto_ancho(n->nombre) + (n->modificado ? r->texto_ancho(" *") : 0), th + 2 };
}

static int nodo_bajo(const Pizarra *p, float wx, float wy) {
  for (int i = p->n_nodos - 1; i >= 0; i--) {
    const NodoSprite *n = &p->nodos[i];
    if (wx >= n->x && wy >= n->y && wx < n->x + (float)n->w && wy < n->y + (float)n->h) return i;
  }
  return -1;
}

static int titulo_bajo(const Pizarra *p, const Renderer2D *r, float sx, float sy, int w, int h) {
  for (int i = p->n_nodos - 1; i >= 0; i--)
    if (dentro(titulo_en_pantalla(p, r, i, w, h), sx, sy)) return i;
  return -1;
}

static void seleccionar(Pizarra *p, int i) {
  if (p->seleccion == i) return;
  p->seleccion = i;
  g_paleta.valida = 0;
}

static void avisar(Pizarra *p, const char *fmt, const char *nombre) {
  snprintf(p->mensaje, sizeof p->mensaje, fmt, nombre);
  p->mensaje_ms = MENSAJE_MS;
}

// ── Deshacer ─────────────────────────────────────────────────

// Una copia completa del nodo antes del trazo. Si el anillo se lleno se
// tira la mas vieja.
static void guardar_deshacer(Pizarra *p, int i) {
  const NodoSprite *n = &p->nodos[i];
  size_t bytes = (size_t)n->w * (size_t)n->h * sizeof(uint32_t);
  uint32_t *copia = malloc(bytes);
  if (!copia) return;
  memcpy(copia, n->pix, bytes);
  if (p->n_deshacer == PIZARRA_DESHACER) {
    free(p->deshacer[0].pix);
    memmove(p->deshacer, p->deshacer + 1, (PIZARRA_DESHACER - 1) * sizeof p->deshacer[0]);
    p->n_deshacer--;
  }
  p->deshacer[p->n_deshacer].nodo = i;
  p->deshacer[p->n_deshacer].pix = copia;
  p->n_deshacer++;
}

// Devuelve el nodo que volvio atras, o -1 si no habia nada.
static int deshacer(Pizarra *p) {
  if (p->n_deshacer == 0) return -1;
  p->n_deshacer--;
  int i = p->deshacer[p->n_deshacer].nodo;
  NodoSprite *n = &p->nodos[i];
  memcpy(n->pix, p->deshacer[p->n_deshacer].pix, (size_t)n->w * (size_t)n->h * sizeof(uint32_t));
  free(p->deshacer[p->n_deshacer].pix);
  p->deshacer[p->n_deshacer].pix = NULL;
  n->modificado = 1;
  return i;
}

// ── Pintar ───────────────────────────────────────────────────

// Aplica la herramienta en un pixel del nodo. 1 si cambio algun pixel.
static int aplicar(Pizarra *p, NodoSprite *n, int x, int y) {
  if (x < 0 || y < 0 || x >= n->w || y >= n->h) return 0;
  uint32_t *px = &n->pix[(size_t)y * (size_t)n->w + (size_t)x];
  if (p->herramienta == HERR_GOTERO) {
    if ((*px >> 24) != 0) p->color = *px | 0xFF000000u;
    return 0;
  }
  uint32_t nuevo = p->herramienta == HERR_GOMA ? 0x00000000u : p->color | 0xFF000000u;
  if (*px == nuevo) return 0;
  *px = nuevo;
  n->modificado = 1;
  return 1;
}

// Bresenham entre la posicion del frame anterior y la de este: un trazo
// rapido no deja huecos aunque el mouse salte varios pixeles.
static int trazar(Pizarra *p, NodoSprite *n, int x0, int y0, int x1, int y1) {
  int dx = abs(x1 - x0), dy = -abs(y1 - y0);
  int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, cambio = 0;
  for (;;) {
    cambio |= aplicar(p, n, x0, y0);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
  return cambio;
}

// ── Abrir y cerrar ───────────────────────────────────────────

// Espera un Pizarra en cero la primera vez (el editor lo tiene estatico).
void pizarra_abrir(Pizarra *p, const Renderer2D *r, const char **rutas, int n) {
  pizarra_cerrar(p, r);
  float x = 0, y = 0, alto_fila = 0;
  for (int i = 0; i < n && p->n_nodos < PIZARRA_NODOS; i++) {
    int w = 0, h = 0;
    uint32_t *pix = r->imagen_cargar(rutas[i], &w, &h);
    if (!pix) continue;
    Textura2D t = r->textura_desde_pixeles(pix, w, h);
    if (t == TEXTURA2D_NULA) { r->imagen_liberar(pix); continue; }

    NodoSprite *nd = &p->nodos[p->n_nodos++];
    snprintf(nd->ruta, sizeof nd->ruta, "%s", rutas[i]);
    const char *barra = strrchr(rutas[i], '/');
    snprintf(nd->nombre, sizeof nd->nombre, "%s", barra ? barra + 1 : rutas[i]);
    nd->pix = pix;
    nd->w = w;
    nd->h = h;
    nd->tex = t;
    nd->modificado = 0;

    // En filas: si no entra en lo que queda de la fila, abajo.
    if (x > 0 && x + (float)w > ANCHO_FILA) {
      x = 0;
      y += alto_fila + SEPARACION;
      alto_fila = 0;
    }
    nd->x = x;
    nd->y = y;
    x += (float)w + SEPARACION;
    if ((float)h > alto_fila) alto_fila = (float)h;
  }
  p->seleccion = p->n_nodos > 0 ? 0 : -1;
  p->herramienta = HERR_LAPIZ;
  p->color = NEGRO2;
  p->zoom = 0;                 // 0 = encuadrar el primer nodo apenas se sepa el tamano de la vista
  g_paleta.valida = 0;
}

void pizarra_cerrar(Pizarra *p, const Renderer2D *r) {
  for (int i = 0; i < p->n_nodos; i++) {
    r->textura_liberar(p->nodos[i].tex);
    r->imagen_liberar(p->nodos[i].pix);
  }
  for (int i = 0; i < p->n_deshacer; i++) free(p->deshacer[i].pix);
  memset(p, 0, sizeof *p);
  p->seleccion = -1;
  g_paleta.valida = 0;
}

// ── Un frame ─────────────────────────────────────────────────

void pizarra_actualizar(Pizarra *p, const Renderer2D *r, const EntradaUI *e, int w, int h, float dt_ms) {
  if (p->mensaje_ms > 0) p->mensaje_ms -= dt_ms;
  if (p->zoom <= 0 && p->n_nodos > 0 && w > 0 && h > 0) encuadrar(p, 0, w, h);
  if (!g_paleta.valida) calcular_paleta(p);

  int sucio = -1;              // nodo a subir a la GPU al final del frame
  float wx, wy;
  a_mundo(p, e->mx, e->my, w, h, &wx, &wy);

  // Teclas
  int ctrl = e->tecla[T2_CTRL];
  if (ctrl && e->tecla_apreto[T2_Z] && p->arrastrando != 3) {
    int i = deshacer(p);
    if (i >= 0) {
      sucio = i;
      if (i == p->seleccion) g_paleta.valida = 0;
      avisar(p, "deshecho en %s", p->nodos[i].nombre);
    }
  }
  if (ctrl && e->tecla_apreto[T2_S] && p->seleccion >= 0) {
    NodoSprite *n = &p->nodos[p->seleccion];
    if (r->imagen_guardar(n->ruta, n->pix, n->w, n->h) == 0) {
      n->modificado = 0;
      avisar(p, "guardado %s", n->nombre);
    } else {
      avisar(p, "no pude guardar %s", n->nombre);
    }
  }
  if (!ctrl) {
    if (e->tecla_apreto[T2_B]) p->herramienta = HERR_LAPIZ;
    if (e->tecla_apreto[T2_E]) p->herramienta = HERR_GOMA;
    if (e->tecla_apreto[T2_I]) p->herramienta = HERR_GOTERO;
  }

  // Rueda: acerca hacia el mouse (el punto de abajo del puntero no se mueve).
  if (e->dentro && e->rueda != 0 && p->zoom > 0) {
    float z = p->zoom * powf(ZOOM_PASO, e->rueda);
    z = fmaxf(ZOOM_MIN, fminf(ZOOM_MAX, z));
    p->cam_x = wx - (e->mx - (float)w / 2) / z;
    p->cam_y = wy - (e->my - (float)h / 2) / z;
    p->zoom = z;
  }

  // Apretar
  Barra b = armar_barra(r, w);
  int sobre_barra = dentro(b.fondo, e->mx, e->my);
  if (e->dentro && p->arrastrando == 0) {
    if (e->apreto[M2_MEDIO] || e->apreto[M2_DER]) {
      p->arrastrando = 1;
      p->agarre_x = e->mx;
      p->agarre_y = e->my;
    } else if (e->apreto[M2_IZQ] && sobre_barra) {
      for (int i = 0; i < 3; i++)
        if (dentro(b.herr[i], e->mx, e->my)) p->herramienta = (Herramienta)i;
      for (int i = 0; i < b.n_muestras; i++)
        if (dentro(b.muestra[i], e->mx, e->my)) {
          p->color = g_paleta.col[i];
          if (p->herramienta != HERR_LAPIZ) p->herramienta = HERR_LAPIZ;
        }
    } else if (e->apreto[M2_IZQ]) {
      int t = titulo_bajo(p, r, e->mx, e->my, w, h);
      int c = nodo_bajo(p, wx, wy);
      if (t >= 0) {
        seleccionar(p, t);
        p->arrastrando = 2;
        p->agarre_x = wx - p->nodos[t].x;
        p->agarre_y = wy - p->nodos[t].y;
      } else if (c >= 0 && c == p->seleccion) {
        NodoSprite *n = &p->nodos[c];
        int px = (int)floorf(wx - n->x), py = (int)floorf(wy - n->y);
        if (p->herramienta != HERR_GOTERO) guardar_deshacer(p, c);
        if (aplicar(p, n, px, py)) sucio = c;
        p->arrastrando = 3;
        p->agarre_x = (float)px;
        p->agarre_y = (float)py;
      } else if (c >= 0) {
        seleccionar(p, c);
      }
    }
  }

  // Arrastrar
  if (p->arrastrando == 1) {
    if (!e->boton[M2_MEDIO] && !e->boton[M2_DER]) {
      p->arrastrando = 0;
    } else {
      float z = zoom_de(p);
      p->cam_x -= (e->mx - p->agarre_x) / z;
      p->cam_y -= (e->my - p->agarre_y) / z;
      p->agarre_x = e->mx;
      p->agarre_y = e->my;
    }
  } else if (p->arrastrando == 2) {
    if (!e->boton[M2_IZQ]) {
      p->arrastrando = 0;
    } else if (p->seleccion >= 0) {
      // Posicion entera: el sprite queda alineado a la grilla de la pizarra.
      p->nodos[p->seleccion].x = roundf(wx - p->agarre_x);
      p->nodos[p->seleccion].y = roundf(wy - p->agarre_y);
    }
  } else if (p->arrastrando == 3) {
    if (!e->boton[M2_IZQ] || p->seleccion < 0) {
      p->arrastrando = 0;
      g_paleta.valida = 0;     // el trazo pudo traer colores nuevos
    } else if (!e->apreto[M2_IZQ]) {
      NodoSprite *n = &p->nodos[p->seleccion];
      int px = (int)floorf(wx - n->x), py = (int)floorf(wy - n->y);
      if (px != (int)p->agarre_x || py != (int)p->agarre_y) {
        if (trazar(p, n, (int)p->agarre_x, (int)p->agarre_y, px, py)) sucio = p->seleccion;
        p->agarre_x = (float)px;
        p->agarre_y = (float)py;
      }
    }
  }

  if (sucio >= 0) r->textura_actualizar(p->nodos[sucio].tex, p->nodos[sucio].pix);
}

// ── Dibujar ──────────────────────────────────────────────────

// Damero detras del sprite, solo la parte visible del nodo. Cada casilla es
// una potencia de 2 de pixeles del arte, la mas chica que mide 12 px o mas
// en pantalla: asi coincide con la grilla de pixeles y no hay miles.
static void damero(const Renderer2D *r, const NodoSprite *n, float vx0, float vy0, float vx1, float vy1,
                   float z) {
  float x0 = fmaxf(n->x, vx0), y0 = fmaxf(n->y, vy0);
  float x1 = fminf(n->x + (float)n->w, vx1), y1 = fminf(n->y + (float)n->h, vy1);
  if (x1 <= x0 || y1 <= y0) return;
  r->rect((Rect2D){ x0, y0, x1 - x0, y1 - y0 }, DAMERO_A);
  float c = 1;
  while (c * z < 12) c *= 2;
  int i0 = (int)floorf((x0 - n->x) / c), i1 = (int)ceilf((x1 - n->x) / c);
  int j0 = (int)floorf((y0 - n->y) / c), j1 = (int)ceilf((y1 - n->y) / c);
  for (int j = j0; j < j1; j++)
    for (int i = i0 + ((i0 + j) & 1); i < i1; i += 2) {
      float cx0 = fmaxf(x0, n->x + (float)i * c), cy0 = fmaxf(y0, n->y + (float)j * c);
      float cx1 = fminf(x1, n->x + (float)(i + 1) * c), cy1 = fminf(y1, n->y + (float)(j + 1) * c);
      if (cx1 > cx0 && cy1 > cy0) r->rect((Rect2D){ cx0, cy0, cx1 - cx0, cy1 - cy0 }, DAMERO_B);
    }
}

// Lineas de 1 px de pantalla entre los pixeles del arte, solo lo visible.
static void grilla(const Renderer2D *r, const NodoSprite *n, float vx0, float vy0, float vx1, float vy1,
                   float z) {
  float x0 = fmaxf(n->x, vx0), y0 = fmaxf(n->y, vy0);
  float x1 = fminf(n->x + (float)n->w, vx1), y1 = fminf(n->y + (float)n->h, vy1);
  if (x1 <= x0 || y1 <= y0) return;
  float grosor = 1 / z;
  int cuantos = 0;
  for (float x = n->x + ceilf(x0 - n->x); x <= x1 && cuantos < MAX_GRILLA; x += 1, cuantos++)
    r->rect((Rect2D){ x, y0, grosor, y1 - y0 }, GRILLA);
  for (float y = n->y + ceilf(y0 - n->y); y <= y1 && cuantos < MAX_GRILLA; y += 1, cuantos++)
    r->rect((Rect2D){ x0, y, x1 - x0, grosor }, GRILLA);
}

static void dibujar_barra(const Pizarra *p, const Renderer2D *r, int w) {
  Barra b = armar_barra(r, w);
  float th = (float)r->texto_alto();
  r->rect(b.fondo, UI_PANEL);
  r->rect((Rect2D){ 0, BARRA_ALTO - 1, (float)w, 1 }, UI_BORDE);
  for (int i = 0; i < 3; i++) {
    int activo = (int)p->herramienta == i;
    caja(r, b.herr[i], activo ? UI_BOTON_ON : UI_BOTON, activo ? UI_ACENTO : UI_BORDE);
    r->texto(b.herr[i].x + 8, b.herr[i].y + (b.herr[i].h - th) / 2, BOTONES_HERR[i],
             activo ? UI_ACENTO : UI_TEXTO);
  }
  caja(r, b.actual, p->color | 0xFF000000u, UI_ACENTO);
  for (int i = 0; i < b.n_muestras; i++) {
    int elegido = (g_paleta.col[i] | 0xFF000000u) == (p->color | 0xFF000000u);
    caja(r, b.muestra[i], g_paleta.col[i], elegido ? UI_ACENTO : UI_BORDE);
  }
}

static void dibujar_estado(const Pizarra *p, const Renderer2D *r, int w, int h) {
  char buf[256];
  float y = (float)h - ESTADO_ALTO;
  r->rect((Rect2D){ 0, y, (float)w, ESTADO_ALTO }, UI_PANEL);
  r->rect((Rect2D){ 0, y, (float)w, 1 }, UI_BORDE);
  if (p->seleccion >= 0) {
    const NodoSprite *n = &p->nodos[p->seleccion];
    snprintf(buf, sizeof buf, "%s  %dx%d  zoom %.2fx%s", n->nombre, n->w, n->h, zoom_de(p),
             n->modificado ? "  * sin guardar" : "");
  } else {
    snprintf(buf, sizeof buf, "sin sprites  zoom %.2fx", zoom_de(p));
  }
  float ty = y + (ESTADO_ALTO - (float)r->texto_alto()) / 2;
  r->texto(MARGEN, ty, buf, UI_TEXTO);
  if (p->mensaje_ms > 0 && p->mensaje[0])
    r->texto((float)w - MARGEN - r->texto_ancho(p->mensaje), ty, p->mensaje, UI_ACENTO);
}

void pizarra_dibujar(const Pizarra *p, const Renderer2D *r, int w, int h) {
  float z = zoom_de(p);
  r->rect((Rect2D){ 0, 0, (float)w, (float)h }, UI_VACIO);

  // Pasada del MUNDO
  float vx0, vy0, vx1, vy1;
  a_mundo(p, 0, 0, w, h, &vx0, &vy0);
  a_mundo(p, (float)w, (float)h, w, h, &vx1, &vy1);
  r->camara(vx0, vy0);
  r->zoom(z);
  for (int i = 0; i < p->n_nodos; i++) {
    const NodoSprite *n = &p->nodos[i];
    if (n->x > vx1 || n->y > vy1 || n->x + (float)n->w < vx0 || n->y + (float)n->h < vy0) continue;
    damero(r, n, vx0, vy0, vx1, vy1, z);
    r->sprite(n->tex, (Rect2D){ 0, 0, (float)n->w, (float)n->h },
              (Rect2D){ n->x, n->y, (float)n->w, (float)n->h }, BLANCO2, 0);
  }
  if (p->seleccion >= 0) {
    const NodoSprite *n = &p->nodos[p->seleccion];
    if (z >= ZOOM_GRILLA) grilla(r, n, vx0, vy0, vx1, vy1, z);
    float g = 2 / z, nw = (float)n->w, nh = (float)n->h;
    r->rect((Rect2D){ n->x - g, n->y - g, nw + 2 * g, g }, UI_ACENTO);
    r->rect((Rect2D){ n->x - g, n->y + nh, nw + 2 * g, g }, UI_ACENTO);
    r->rect((Rect2D){ n->x - g, n->y, g, nh }, UI_ACENTO);
    r->rect((Rect2D){ n->x + nw, n->y, g, nh }, UI_ACENTO);
  }

  // Pasada de PANTALLA: texto a escala 1
  r->camara(0, 0);
  r->zoom(1);
  for (int i = 0; i < p->n_nodos; i++) {
    Rect2D t = titulo_en_pantalla(p, r, i, w, h);
    if (t.x > (float)w || t.y > (float)h || t.x + t.w < 0 || t.y + t.h < 0) continue;
    char buf[80];
    snprintf(buf, sizeof buf, "%s%s", p->nodos[i].nombre, p->nodos[i].modificado ? " *" : "");
    r->texto(t.x, t.y, buf, i == p->seleccion ? UI_ACENTO : UI_TEXTO);
  }
  dibujar_barra(p, r, w);
  dibujar_estado(p, r, w, h);
}

int pizarra_inspeccionar(const Pizarra *p, char lineas[][64], int max) {
  int n = 0;
  if (max <= 0) return 0;
  if (p->seleccion < 0) {
    snprintf(lineas[n++], 64, "sin sprite seleccionado");
    return n;
  }
  const NodoSprite *s = &p->nodos[p->seleccion];
  int pendientes = 0;
  for (int i = 0; i < p->n_nodos; i++) pendientes += p->nodos[i].modificado != 0;
  if (n < max) snprintf(lineas[n++], 64, "archivo      %.50s", s->nombre);
  if (n < max) snprintf(lineas[n++], 64, "tamano       %d x %d", s->w, s->h);
  if (n < max) snprintf(lineas[n++], 64, "zoom         %.2fx", zoom_de(p));
  if (n < max) snprintf(lineas[n++], 64, "herramienta  %s", NOMBRES_HERR[p->herramienta]);
  if (n < max) snprintf(lineas[n++], 64, "color        #%08X", (unsigned)p->color);
  if (n < max) snprintf(lineas[n++], 64, "cambios      %s", s->modificado ? "sin guardar" : "guardado");
  if (n < max && pendientes > 0) snprintf(lineas[n++], 64, "sin guardar  %d sprite%s", pendientes,
                                          pendientes == 1 ? "" : "s");
  return n;
}
