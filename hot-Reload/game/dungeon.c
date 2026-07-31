// dungeon.c — la puerta de entrada de game.so y nada mas.
//
// Aca viven las tres funciones que el host resuelve con dlsym en cada recarga
// (ver platform.h). No hay main(): game.so es una biblioteca, no un programa.
// El movimiento esta en playerC.c, la rejilla del grupo en formacion.c, y los
// tipos en dungeon.h.
//
// Controles, como en Age of Empires:
//   boton izquierdo, arrastrando -> selecciona todo lo que quede adentro
//   boton izquierdo, un toque    -> selecciona solo lo que este bajo el cursor
//   boton derecho                -> manda a los seleccionados a ese punto
//   q                            -> salir

#include "dungeon.h"
#include "formacion.h"
#include "platform.h"
#include "playerC.h"

// La ultima fila del framebuffer la pisa el HUD del host (host.c dibuja los fps
// ahi), asi que el area util termina una fila antes.
#define PISO_ALTO (FB_ALTO - 1)

// Cuanto se separa el terreno del borde del framebuffer. La pared se dibuja en
// ese margen, justo por fuera del area caminable.
#define MARGEN 2

#define SOLDADOS_INICIALES 6

static int recortar(int v, int minimo, int maximo) {
  if (v < minimo)
    return minimo;
  if (v > maximo)
    return maximo;
  return v;
}

static void terreno_recortar(const Terreno *t, int *x, int *y) {
  *x = recortar(*x, t->x0, t->x1);
  *y = recortar(*y, t->y0, t->y1);
}

// El rectangulo se guarda como "donde empezo" y "donde esta ahora", asi que si
// arrastras hacia arriba o hacia la izquierda, x0 termina siendo mayor que x1.
// Ordenar los limites antes de comparar es lo que hace que la seleccion
// funcione en las cuatro diagonales y no solo arrastrando hacia abajo.
static int dentro_del_recuadro(const Seleccion *s, float px, float py) {
  int izq = s->x0 < s->x1 ? s->x0 : s->x1;
  int der = s->x0 < s->x1 ? s->x1 : s->x0;
  int arr = s->y0 < s->y1 ? s->y0 : s->y1;
  int aba = s->y0 < s->y1 ? s->y1 : s->y0;

  int x = (int)px;
  int y = (int)py;
  return x >= izq && x <= der && y >= arr && y <= aba;
}

// --- game_init ---------------------------------------------------------------

void game_init(void *mem, size_t size) {
  if (size < sizeof(Estado))
    return; // el host presta 16 MB; esto no deberia pasar nunca

  Estado *e = (Estado *)mem;

  // El host entrega el bloque en cero la primera vez, pero lo conserva tal cual
  // entre recargas. Este flag distingue "arranque de cero" de "recarga en
  // caliente", y es lo que evita que se te reinicie la partida al guardar.
  if (e->inicializado)
    return;

  e->terreno.x0 = MARGEN;
  e->terreno.y0 = MARGEN;
  e->terreno.x1 = FB_ANCHO - 1 - MARGEN;
  e->terreno.y1 = PISO_ALTO - 1 - MARGEN;

  float centro_x = (e->terreno.x0 + e->terreno.x1) / 2.0f;
  float centro_y = (e->terreno.y0 + e->terreno.y1) / 2.0f;

  // Arrancan ya formados, uno al lado del otro. La misma funcion que usa la
  // orden de movimiento, asi que la formacion inicial y la de destino tienen
  // exactamente la misma forma.
  e->cant_soldados = SOLDADOS_INICIALES;
  for (int i = 0; i < e->cant_soldados; i++) {
    float x, y;
    formacion_slot(i, e->cant_soldados, centro_x, centro_y, &x, &y);

    int cx = (int)x, cy = (int)y;
    terreno_recortar(&e->terreno, &cx, &cy);
    soldado_init(&e->soldados[i], (float)cx, (float)cy);
  }

  e->seleccion.activa = 0;
  e->inicializado = 1;
}

// --- game_update -------------------------------------------------------------

// Marca como seleccionado todo soldado que caiga adentro del recuadro. Empieza
// limpiando: una seleccion nueva reemplaza a la anterior, no se suma.
static void aplicar_seleccion(Estado *e) {
  for (int i = 0; i < e->cant_soldados; i++)
    e->soldados[i].seleccionado =
        dentro_del_recuadro(&e->seleccion, e->soldados[i].x, e->soldados[i].y);
}

// Manda a los seleccionados hacia (cx, cy), cada uno a su lugar en la rejilla.
static void ordenar_movimiento(Estado *e, int cx, int cy) {
  // Primero hay que saber cuantos van, porque la forma de la rejilla depende
  // del total. Por eso son dos recorridas y no una.
  int total = 0;
  for (int i = 0; i < e->cant_soldados; i++)
    if (e->soldados[i].seleccionado)
      total++;

  if (total == 0)
    return;

  int slot = 0;
  for (int i = 0; i < e->cant_soldados; i++) {
    if (!e->soldados[i].seleccionado)
      continue;

    float x, y;
    formacion_slot(slot, total, (float)cx, (float)cy, &x, &y);
    slot++;

    // Cada puesto de la formacion se recorta por separado: si el grupo va
    // contra una pared, los de afuera se apoyan sobre el borde en vez de
    // salirse del terreno.
    int dx = (int)x, dy = (int)y;
    terreno_recortar(&e->terreno, &dx, &dy);
    soldado_ordenar_ir(&e->soldados[i], dx, dy);
  }
}

int game_update(void *mem, size_t size, Input in, float dt) {
  if (size < sizeof(Estado))
    return 0;

  Estado *e = (Estado *)mem;

  // --- Boton izquierdo: seleccionar ---
  if (in.mouse.izq_apretado) {
    e->seleccion.activa = 1;
    e->seleccion.x0 = e->seleccion.x1 = in.mouse.x;
    e->seleccion.y0 = e->seleccion.y1 = in.mouse.y;
  }

  if (e->seleccion.activa && in.mouse.arrastrando) {
    e->seleccion.x1 = in.mouse.x;
    e->seleccion.y1 = in.mouse.y;
  }

  if (in.mouse.izq_soltado && e->seleccion.activa) {
    e->seleccion.x1 = in.mouse.x;
    e->seleccion.y1 = in.mouse.y;
    // Un click sin arrastre es un recuadro de una sola celda, y la misma
    // funcion lo resuelve: selecciona lo que este justo ahi. No hace falta un
    // caso especial.
    aplicar_seleccion(e);
    e->seleccion.activa = 0;
  }

  // --- Boton derecho: mover ---
  if (in.mouse.der_apretado) {
    int x = in.mouse.x;
    int y = in.mouse.y;
    terreno_recortar(&e->terreno, &x, &y);
    ordenar_movimiento(e, x, y);
  }

  // --- Teclado ---
  // Solo salir. El movimiento por teclas quedo en tecla_a_direccion()
  // (playerC.c), compilada y lista, pero hoy no se llama.
  for (int i = 0; i < in.n; i++) {
    if (in.bytes[i] == 'q')
      return 0; // devolver 0 corta el bucle del host y sale
  }

  for (int i = 0; i < e->cant_soldados; i++)
    soldado_update(&e->soldados[i], dt);

  return 1;
}

// --- game_render -------------------------------------------------------------

static void dibujar_paredes(Framebuffer *fb, const Terreno *t) {
  for (int x = t->x0 - 1; x <= t->x1 + 1; x++) {
    fb_poner(fb, x, t->y0 - 1, '#', 90);
    fb_poner(fb, x, t->y1 + 1, '#', 90);
  }
  for (int y = t->y0 - 1; y <= t->y1 + 1; y++) {
    fb_poner(fb, t->x0 - 1, y, '#', 90);
    fb_poner(fb, t->x1 + 1, y, '#', 90);
  }
}

// Solo el contorno, no relleno: si pintaras el interior taparias justo a las
// unidades que estas tratando de seleccionar.
static void dibujar_recuadro(Framebuffer *fb, const Seleccion *s) {
  int izq = s->x0 < s->x1 ? s->x0 : s->x1;
  int der = s->x0 < s->x1 ? s->x1 : s->x0;
  int arr = s->y0 < s->y1 ? s->y0 : s->y1;
  int aba = s->y0 < s->y1 ? s->y1 : s->y0;

  for (int x = izq; x <= der; x++) {
    fb_poner(fb, x, arr, '-', 32);
    fb_poner(fb, x, aba, '-', 32);
  }
  for (int y = arr; y <= aba; y++) {
    fb_poner(fb, izq, y, '|', 32);
    fb_poner(fb, der, y, '|', 32);
  }
}

void game_render(void *mem, size_t size, Framebuffer *fb) {
  if (size < sizeof(Estado))
    return;

  Estado *e = (Estado *)mem;
  fb_limpiar(fb);

  dibujar_paredes(fb, &e->terreno);

  // Marcas de destino primero, para que ningun soldado quede tapado por una.
  for (int i = 0; i < e->cant_soldados; i++)
    if (e->soldados[i].moviendose)
      fb_poner(fb, (int)e->soldados[i].destino_x, (int)e->soldados[i].destino_y,
               'x', 33);

  // La posicion es float y el framebuffer trabaja en celdas enteras: el casteo
  // a int es el unico lugar donde se pierden los decimales, y esta bien que sea
  // asi. Los decimales siguen acumulandose en el Estado.
  for (int i = 0; i < e->cant_soldados; i++) {
    const Soldado *s = &e->soldados[i];
    fb_poner(fb, (int)s->x, (int)s->y, s->seleccionado ? 'O' : '@',
             s->seleccionado ? 32 : 36);
  }

  // El recuadro va ultimo para que se vea por encima de todo mientras arrastras.
  if (e->seleccion.activa)
    dibujar_recuadro(fb, &e->seleccion);

  fb_texto(fb, 0, 0, "arrastra izq = seleccionar  |  der = mover  |  q = salir",
           90);
}
