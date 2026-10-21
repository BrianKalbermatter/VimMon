// ============================================================
// VimMon — VISTA ESCENA y HIERARCHY
//
// La camara de la vista no es la del juego: es la del editor. Guarda el
// punto del mundo que cae en el CENTRO de la vista y el zoom:
//
//   pantalla = (mundo - esquina) * zoom
//   esquina  = centro - (w, h) / (2 * zoom)
//
// Con eso las dos cuentas que importan salen solas: que punto del mundo
// esta bajo el mouse, y donde tiene que quedar la camara para que ese
// punto SIGA bajo el mouse despues de un zoom o mientras se arrastra.
// ============================================================

#include "vista_escena.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define ZOOM_MIN    0.02f
#define ZOOM_MAX    8.0f
#define PASO_RUEDA  1.15f   // cuanto acerca cada muesca de la rueda
#define SANGRIA     14      // px por nivel del arbol
#define FILAS_RUEDA 3       // filas que corre cada muesca en la Hierarchy

// El contrato de dibujar no recibe la entrada, y el overlay (coordenadas
// del mouse) y el hover de la Hierarchy la necesitan: actualizar la deja
// aca. Hay una sola vista Escena y una sola Hierarchy en el editor.
static float g_mx, g_my;        // mouse en px de la vista Escena
static int   g_dentro;          // el mouse esta sobre la vista Escena
static int   g_fila_encima = -1; // fila de la Hierarchy bajo el mouse

static int dentro(Rect2D r, float x, float y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

static float limitar(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }

// ── Camara ───────────────────────────────────────────────────

// Lo que se ve del mundo, en px de mundo.
static Rect2D visible(const VistaEscena *v, int w, int h) {
  float vw = (float)w / v->zoom, vh = (float)h / v->zoom;
  return (Rect2D){ v->cam_x - vw / 2, v->cam_y - vh / 2, vw, vh };
}

static void a_mundo(const VistaEscena *v, float sx, float sy, int w, int h, float *x, float *y) {
  *x = v->cam_x + (sx - (float)w / 2) / v->zoom;
  *y = v->cam_y + (sy - (float)h / 2) / v->zoom;
}

// Mueve la camara para que el punto del mundo (x, y) quede bajo (sx, sy).
static void fijar_bajo(VistaEscena *v, float x, float y, float sx, float sy, int w, int h) {
  v->cam_x = x - (sx - (float)w / 2) / v->zoom;
  v->cam_y = y - (sy - (float)h / 2) / v->zoom;
}

static int se_tocan(Rect2D a, Rect2D b) {
  return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
}

// Una caja que tapa toda la vista (la raiz del mundo, un grupo enorme) no
// dice nada: solo se dibuja si esta seleccionada, y no se elige con clic.
static int tapa_la_vista(Rect2D c, Rect2D vis) { return c.w >= vis.w && c.h >= vis.h; }

static void encuadrar(VistaEscena *v, int w, int h) {
  if (v->seleccion < 0) return;
  Rect2D c = v->objs[v->seleccion].caja;
  v->cam_x = c.x + c.w / 2;
  v->cam_y = c.y + c.h / 2;
  if (c.w <= 0 || c.h <= 0) return;
  // Que la caja ocupe la mitad de la vista en su lado mas justo.
  float zx = (float)w * 0.5f / c.w, zy = (float)h * 0.5f / c.h;
  v->zoom = limitar(zx < zy ? zx : zy, ZOOM_MIN, ZOOM_MAX);
}

// ── Escena ───────────────────────────────────────────────────

void escena_iniciar(VistaEscena *v) {
  memset(v, 0, sizeof *v);
  v->zoom = 1;
  v->seleccion = -1;
}

void escena_refrescar(VistaEscena *v, const Modulo2D *m) {
  int n = m && m->escena ? m->escena(v->objs, ESCENA_MAX) : 0;
  if (n < 0) n = 0;
  if (n > ESCENA_MAX) n = ESCENA_MAX;
  v->n_objs = n;
  // El contrato pide padre ANTERIOR; si el juego se equivoca, el objeto
  // queda como raiz en vez de armar un ciclo.
  for (int i = 0; i < n; i++) {
    if (v->objs[i].padre < -1 || v->objs[i].padre >= i) v->objs[i].padre = -1;
    v->objs[i].nombre[sizeof v->objs[i].nombre - 1] = 0;
    v->objs[i].tipo[sizeof v->objs[i].tipo - 1] = 0;
  }
  if (v->seleccion >= n) v->seleccion = -1;
  if (v->seleccion < 0 && v->arrastrando == 2) v->arrastrando = 0;
  if (n == 0) {
    v->encuadrado = 0;   // la proxima partida vuelve a centrar
    return;
  }
  if (v->encuadrado) return;
  // Primera vez con objetos: al primer movible (el jugador), a tamano real.
  for (int i = 0; i < n; i++) {
    if (!v->objs[i].movible) continue;
    Rect2D c = v->objs[i].caja;
    v->cam_x = c.x + c.w / 2;
    v->cam_y = c.y + c.h / 2;
    v->zoom = 1;
    v->encuadrado = 1;
    return;
  }
  // Nada movible: el centro del mundo, si el juego lo dice.
  if (m->mundo_tamano) {
    float mw = 0, mh = 0;
    m->mundo_tamano(&mw, &mh);
    v->cam_x = mw / 2;
    v->cam_y = mh / 2;
  }
  v->encuadrado = 1;
}

// El objeto mas chico bajo el punto (x, y) del mundo, o -1.
static int elegir(const VistaEscena *v, float x, float y, Rect2D vis) {
  int mejor = -1;
  float area = 0;
  for (int i = 0; i < v->n_objs; i++) {
    Rect2D c = v->objs[i].caja;
    if (c.w <= 0 || c.h <= 0 || tapa_la_vista(c, vis) || !dentro(c, x, y)) continue;
    if (mejor < 0 || c.w * c.h < area) {
      mejor = i;
      area = c.w * c.h;
    }
  }
  return mejor;
}

void escena_actualizar(VistaEscena *v, const Modulo2D *m, const EntradaUI *e, int w, int h) {
  if (w <= 0 || h <= 0) return;
  g_mx = e->mx;
  g_my = e->my;
  g_dentro = e->dentro;
  float mx, my;
  a_mundo(v, e->mx, e->my, w, h, &mx, &my);

  // Arrastres en curso: siguen aunque el mouse salga de la vista.
  if (v->arrastrando == 1) {
    if (e->boton[M2_MEDIO] || e->boton[M2_DER]) fijar_bajo(v, v->agarre_x, v->agarre_y, e->mx, e->my, w, h);
    else v->arrastrando = 0;
  } else if (v->arrastrando == 2) {
    if (e->boton[M2_IZQ] && v->seleccion >= 0 && m && m->mover_objeto) {
      // agarre = del centro de la caja al punto agarrado: el objeto no salta.
      m->mover_objeto(v->seleccion, mx - v->agarre_x, my - v->agarre_y);
    } else {
      v->arrastrando = 0;
    }
  }

  // Rueda: acerca hacia el mouse (el punto bajo el mouse no se mueve).
  if (e->dentro && e->rueda != 0) {
    v->zoom = limitar(v->zoom * powf(PASO_RUEDA, e->rueda), ZOOM_MIN, ZOOM_MAX);
    fijar_bajo(v, mx, my, e->mx, e->my, w, h);
    a_mundo(v, e->mx, e->my, w, h, &mx, &my);
  }

  if (e->dentro && v->arrastrando == 0) {
    if (e->apreto[M2_MEDIO] || e->apreto[M2_DER]) {
      v->arrastrando = 1;
      v->agarre_x = mx;   // aca agarre es el punto del mundo que se agarro
      v->agarre_y = my;
    } else if (e->apreto[M2_IZQ]) {
      v->seleccion = elegir(v, mx, my, visible(v, w, h));
      if (v->seleccion >= 0 && v->objs[v->seleccion].movible && m && m->mover_objeto) {
        Rect2D c = v->objs[v->seleccion].caja;
        v->arrastrando = 2;
        v->agarre_x = mx - (c.x + c.w / 2);
        v->agarre_y = my - (c.y + c.h / 2);
      }
    }
  }

  if (e->tecla_apreto[T2_F]) encuadrar(v, w, h);
}

// Contorno de 1 px de pantalla: en el mundo mide 1/zoom.
static void contorno(const Renderer2D *r, Rect2D c, float g, uint32_t color) {
  r->rect((Rect2D){ c.x, c.y, c.w, g }, color);
  r->rect((Rect2D){ c.x, c.y + c.h - g, c.w, g }, color);
  r->rect((Rect2D){ c.x, c.y + g, g, c.h - 2 * g }, color);
  r->rect((Rect2D){ c.x + c.w - g, c.y + g, g, c.h - 2 * g }, color);
}

void escena_dibujar(VistaEscena *v, const Modulo2D *m, const Renderer2D *r, int w, int h) {
  float W = (float)w, H = (float)h;
  char buf[96];
  r->camara(0, 0);
  r->zoom(1);
  r->rect((Rect2D){ 0, 0, W, H }, UI_VACIO);
  if (w <= 0 || h <= 0) return;

  // El mundo, con la camara del editor.
  Rect2D vis = visible(v, w, h);
  // La esquina cae en un px entero de pantalla: con zoom fraccionario y
  // muestreo nearest, medio px de corrimiento deja costuras oscuras entre
  // las filas de baldosas.
  vis.x = roundf(vis.x * v->zoom) / v->zoom;
  vis.y = roundf(vis.y * v->zoom) / v->zoom;
  r->camara(vis.x, vis.y);
  r->zoom(v->zoom);
  if (m && m->dibujar_mundo) {
    m->dibujar_mundo(r, vis, v->zoom);
  } else if (m && m->mundo_tamano) {
    float mw = 0, mh = 0;
    m->mundo_tamano(&mw, &mh);
    r->rect((Rect2D){ 0, 0, mw, mh }, UI_PANEL);
  }

  // Las cajas; la seleccionada al final, para que quede arriba.
  float g = 1.0f / v->zoom;
  for (int i = 0; i < v->n_objs; i++) {
    Rect2D c = v->objs[i].caja;
    if (i == v->seleccion || c.w <= 0 || c.h <= 0) continue;
    if (!se_tocan(c, vis) || tapa_la_vista(c, vis)) continue;
    contorno(r, c, g, UI_TENUE);
  }
  int sel = v->seleccion;
  Rect2D cs = sel >= 0 ? v->objs[sel].caja : (Rect2D){ 0, 0, 0, 0 };
  if (sel >= 0 && cs.w > 0 && cs.h > 0) {
    r->rect(cs, ARGB(0x30, 0xfb, 0xb9, 0x54));
    contorno(r, cs, g, UI_ACENTO);
  }

  // Encima, en px de la vista.
  r->camara(0, 0);
  r->zoom(1);
  float th = (float)r->texto_alto();
  if (sel >= 0) {
    const char *nom = v->objs[sel].nombre;
    float tw = r->texto_ancho(nom);
    // Arriba de la caja; si no entra, adentro de la vista.
    float lx = (cs.x - vis.x) * v->zoom, ly = (cs.y - vis.y) * v->zoom - th - 4;
    lx = limitar(lx, 4, W - tw - 8);
    ly = limitar(ly, 4, H - th - 8);
    r->rect((Rect2D){ lx - 3, ly - 1, tw + 6, th + 2 }, ARGB(0xC0, 0x0d, 0x0b, 0x12));
    r->texto(lx, ly, nom, UI_ACENTO);
  }

  float mx, my;
  a_mundo(v, g_mx, g_my, w, h, &mx, &my);
  if (g_dentro) snprintf(buf, sizeof buf, "zoom %.0f%%   x %.0f  y %.0f", v->zoom * 100, mx, my);
  else snprintf(buf, sizeof buf, "zoom %.0f%%", v->zoom * 100);
  r->rect((Rect2D){ 6, 6, r->texto_ancho(buf) + 12, th + 6 }, ARGB(0xC0, 0x1e, 0x1a, 0x25));
  r->texto(12, 9, buf, UI_TEXTO);

  const char *ayuda = "rueda: zoom  medio/der: mover vista  F: encuadrar";
  r->rect((Rect2D){ 6, H - th - 11, r->texto_ancho(ayuda) + 12, th + 6 }, ARGB(0xC0, 0x1e, 0x1a, 0x25));
  r->texto(12, H - th - 8, ayuda, UI_TENUE);
}

// ── Hierarchy ────────────────────────────────────────────────
//
// La lista es plana y el padre siempre va antes que el hijo, asi que una
// sola pasada en orden ya da el arbol en el orden de lectura: cada objeto
// aparece debajo de su padre. Solo hay que saltear a los que tienen algun
// antepasado plegado.

static int profundidad(const VistaEscena *v, int i) {
  int d = 0;
  for (int p = v->objs[i].padre; p >= 0 && d < ESCENA_MAX; p = v->objs[p].padre) d++;
  return d;
}

static int oculto(const VistaEscena *v, int i) {
  int n = 0;
  for (int p = v->objs[i].padre; p >= 0 && n < ESCENA_MAX; p = v->objs[p].padre, n++)
    if (v->plegado[p]) return 1;
  return 0;
}

// Los objetos que se ven, en orden. Devuelve cuantos.
static int filas_visibles(const VistaEscena *v, int *filas) {
  int n = 0;
  for (int i = 0; i < v->n_objs; i++)
    if (!oculto(v, i)) filas[n++] = i;
  return n;
}

static void marcar_padres(const VistaEscena *v, unsigned char *tiene_hijos) {
  memset(tiene_hijos, 0, ESCENA_MAX);
  for (int i = 0; i < v->n_objs; i++)
    if (v->objs[i].padre >= 0) tiene_hijos[v->objs[i].padre] = 1;
}

static float alto_fila(int alto_texto) { return (float)alto_texto + 6; }

// Cuantas filas enteras entran en la zona.
static int filas_que_entran(Rect2D zona, int alto_texto) {
  int n = (int)((zona.h - 4) / alto_fila(alto_texto));
  return n > 0 ? n : 0;
}

void jerarquia_actualizar(VistaEscena *v, const EntradaUI *e, Rect2D zona, int alto_texto) {
  int filas[ESCENA_MAX];
  int n = filas_visibles(v, filas);
  int entran = filas_que_entran(zona, alto_texto);
  int adentro = dentro(zona, e->mx, e->my);

  if (adentro && e->rueda != 0) v->scroll -= (int)(e->rueda * FILAS_RUEDA);
  int tope = n - entran;
  if (v->scroll > tope) v->scroll = tope;
  if (v->scroll < 0) v->scroll = 0;

  g_fila_encima = -1;
  if (!adentro || n == 0) return;
  float af = alto_fila(alto_texto);
  int k = (int)((e->my - zona.y - 2) / af);
  if (e->my < zona.y + 2 || k >= entran || v->scroll + k >= n) return;
  g_fila_encima = v->scroll + k;
  if (!e->apreto[M2_IZQ]) return;

  int i = filas[g_fila_encima];
  unsigned char tiene_hijos[ESCENA_MAX];
  marcar_padres(v, tiene_hijos);
  // El triangulo pliega; el resto de la fila selecciona.
  float tx = zona.x + 6 + (float)(profundidad(v, i) * SANGRIA);
  if (tiene_hijos[i] && e->mx >= tx - 4 && e->mx < tx + SANGRIA) v->plegado[i] = !v->plegado[i];
  else v->seleccion = i;
}

// Corta `s` para que entre en `ancho` px (sin "...": es un panel chico).
static void cortar(const Renderer2D *r, char *s, float ancho) {
  size_t n = strlen(s);
  while (n > 0 && r->texto_ancho(s) > ancho) s[--n] = 0;
}

void jerarquia_dibujar(const VistaEscena *v, const Renderer2D *r, Rect2D zona) {
  int at = r->texto_alto();
  if (v->n_objs == 0) {
    r->texto(zona.x + 8, zona.y + 4, "(sin escena: dale Play)", UI_TENUE);
    return;
  }
  int filas[ESCENA_MAX];
  unsigned char tiene_hijos[ESCENA_MAX];
  int n = filas_visibles(v, filas);
  marcar_padres(v, tiene_hijos);
  int entran = filas_que_entran(zona, at);
  float af = alto_fila(at);

  for (int k = 0; k < entran && v->scroll + k < n; k++) {
    int f = v->scroll + k, i = filas[f];
    const Objeto2D *o = &v->objs[i];
    float y = zona.y + 2 + (float)k * af;
    Rect2D fila = { zona.x + 2, y, zona.w - 4, af };
    int elegido = i == v->seleccion;
    if (elegido) r->rect(fila, UI_BOTON_ON);
    else if (f == g_fila_encima) r->rect(fila, UI_BOTON);

    float ty = y + (af - (float)at) / 2;
    float tx = zona.x + 6 + (float)(profundidad(v, i) * SANGRIA);
    if (tiene_hijos[i]) r->texto(tx, ty, v->plegado[i] ? ">" : "v", UI_TENUE);
    float nx = tx + SANGRIA, fin = zona.x + zona.w - 8;

    // El tipo a la derecha, si queda lugar despues del nombre.
    float tipo_w = r->texto_ancho(o->tipo);
    float nombre_w = r->texto_ancho(o->nombre);
    int con_tipo = o->tipo[0] && nx + nombre_w + 12 + tipo_w <= fin;
    if (con_tipo) r->texto(fin - tipo_w, ty, o->tipo, UI_TENUE);

    char nom[sizeof o->nombre];
    memcpy(nom, o->nombre, sizeof nom);
    cortar(r, nom, fin - nx);
    if (nx < fin) r->texto(nx, ty, nom, elegido ? UI_ACENTO : UI_TEXTO);
  }
}
