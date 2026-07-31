// dungeon.c — la puerta de entrada de game.so y nada mas.
//
// Aca viven las tres funciones que el host resuelve con dlsym en cada recarga
// (ver platform.h). No hay main(): game.so es una biblioteca, no un programa.
// El movimiento esta en playerC.c, la rejilla del grupo en formacion.c, la
// pelea en combate.c, y los tipos en dungeon.h.
//
// Controles, como en Age of Empires:
//   boton izquierdo, arrastrando -> selecciona todo lo que quede adentro
//   boton izquierdo, un toque    -> selecciona solo lo que este bajo el cursor
//   boton derecho                -> manda a los seleccionados a ese punto
//   q                            -> salir

#include "combate.h"
#include "dungeon.h"
#include "formacion.h"
#include "platform.h"
#include "playerC.h"

#include <stdio.h>  // snprintf
#include <stdlib.h> // rand

// Reparto vertical del framebuffer, de arriba hacia abajo:
//
//   0 .. piso_alto-1       el mapa, con sus paredes; arranca en la fila 0
//   piso_alto .. +5        el panel de abajo (tres cajas)
//   alto-1                 HUD del host (fps), que lo dibuja el, no nosotros
//
// Toda la informacion de estado vive en el panel de abajo. Arriba no hay nada:
// el mapa aprovecha hasta la primera fila. El alto ya no es una constante: sale
// de lo que el host midio de la terminal, asi que el mapa ocupa todo lo que
// haya.
#define PANEL_ALTO 6

// Cuanto se separa el terreno del borde del framebuffer. La pared se dibuja en
// ese margen, justo por fuera del area caminable.
#define MARGEN 2

// Ancho en celdas de la barra de vida que se dibuja encima de cada unidad.
#define BARRA_ANCHO 3

// El orden TIENE que coincidir con el enum TipoRecurso de dungeon.h. Es el
// unico lugar donde hay que cuidarlo.
const char *const RECURSO_NOMBRE[REC_CANTIDAD] = {
    "MADERA", "HIERRO", "COBRE",  "COMIDA", "ORO",
    "TITANIO", "BREA",  "PIEDRA", "CARBON", "HILO",
};

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

// --- Camara ------------------------------------------------------------------

// Deja la camara adentro del mundo. Si el mundo fuera mas chico que la ventana
// el maximo daria negativo, y sin el recorte a 0 la camara se iria a coordenadas
// negativas mostrando vacio.
static void camara_recortar(Estado *e) {
  int max_x = MUNDO_ANCHO - e->ancho;
  int max_y = MUNDO_ALTO - e->piso_alto;
  if (max_x < 0)
    max_x = 0;
  if (max_y < 0)
    max_y = 0;
  e->cam_x = recortar(e->cam_x, 0, max_x);
  e->cam_y = recortar(e->cam_y, 0, max_y);
}

// Pantalla -> mundo. Todo lo que llega del mouse pasa por aca antes de tocar
// nada del juego: el mouse informa celdas de pantalla, y el juego solo entiende
// coordenadas de mundo.
static void a_mundo(const Estado *e, int sx, int sy, int *wx, int *wy) {
  *wx = sx + e->cam_x;
  *wy = sy + e->cam_y;
}

// El click cae sobre el mapa, o mas abajo, sobre el panel? Un click en el panel
// no tiene que mover unidades ni plantar cuarteles.
static int en_el_mapa(const Estado *e, int sy) { return sy < e->piso_alto; }


// El rectangulo se guarda como "donde empezo" y "donde esta ahora", asi que si
// arrastras hacia arriba o hacia la izquierda, x0 termina siendo mayor que x1.
// Ordenar los limites antes de comparar es lo que hace que la seleccion
// funcione en las cuatro diagonales y no solo arrastrando hacia abajo.
// El recuadro esta en coordenadas de PANTALLA y las unidades viven en
// coordenadas de MUNDO. Sumarle la camara a los limites convierte el recuadro,
// que son dos puntos, en vez de convertir cada unidad: menos cuentas y un solo
// lugar donde equivocarse.
static int dentro_del_recuadro(const Seleccion *s, int cam_x, int cam_y,
                               float px, float py) {
  int izq = (s->x0 < s->x1 ? s->x0 : s->x1) + cam_x;
  int der = (s->x0 < s->x1 ? s->x1 : s->x0) + cam_x;
  int arr = (s->y0 < s->y1 ? s->y0 : s->y1) + cam_y;
  int aba = (s->y0 < s->y1 ? s->y1 : s->y0) + cam_y;

  int x = (int)px;
  int y = (int)py;
  return x >= izq && x <= der && y >= arr && y <= aba;
}

// --- game_init ---------------------------------------------------------------

void game_init(void *mem, size_t size, int ancho, int alto) {
  if (size < sizeof(Estado))
    return; // el host presta 16 MB; esto no deberia pasar nunca

  Estado *e = (Estado *)mem;

  // El host entrega el bloque en cero la primera vez, pero lo conserva tal cual
  // entre recargas. Este flag distingue "arranque de cero" de "recarga en
  // caliente", y es lo que evita que se te reinicie la partida al guardar.
  if (e->inicializado)
    return;

  // El layout entero sale del tamanio que midio el host. Guardarlo en el Estado
  // (y no recalcularlo en cada funcion) hace que todo el juego vea el mismo
  // numero, aunque se recargue en caliente.
  e->ancho = ancho;
  e->alto = alto;
  e->piso_alto = alto - 1 - PANEL_ALTO;

  // El terreno ya no depende de la pantalla: es el MUNDO. La pantalla es una
  // ventana que se mueve por encima. Se deja una celda de margen en cada lado
  // para que la pared entre dentro del mundo.
  e->terreno.x0 = 1;
  e->terreno.y0 = 1;
  e->terreno.x1 = MUNDO_ANCHO - 2;
  e->terreno.y1 = MUNDO_ALTO - 2;

  e->cam_x = 0;
  e->cam_y = 0;
  e->pan_activo = 0;
  e->hubo_arrastre = 0;

  // La apertura ocupa cinco filas en el medio de la pared derecha.
  int medio = (e->terreno.y0 + e->terreno.y1) / 2;
  e->terreno.apertura_y0 = medio - 2;
  e->terreno.apertura_y1 = medio + 2;

  // El campo arranca VACIO: no hay soldados hasta que levantes un cuartel y
  // este entrene el primero.
  e->cant_soldados = 0;
  e->cant_cuarteles = 0;
  e->modo_construir = 0;

  e->cant_enemigos = 0;
  e->spawn_espera = 1.0f;
  e->preparacion = PREPARACION_PRIMERA;
  e->oleada = 0;
  e->por_aparecer = ENEMIGOS_OLEADA_BASE;

  for (int i = 0; i < MAX_PROYECTILES; i++)
    e->proyectiles[i].activo = 0;

  // Todos los recursos arrancan en cero, y la poblacion tambien porque no hay
  // una sola unidad en el campo.
  for (int i = 0; i < REC_CANTIDAD; i++)
    e->recursos.cantidad[i] = 0;
  e->recursos.poblacion = 0;

  // Los tesoros se reparten por la mitad derecha del mapa. Del lado izquierdo
  // no va ninguno: ahi es donde levantas el primer cuartel, y un tesoro regalado
  // al lado de casa no obliga a salir a buscarlo.
  int desde_x = (e->terreno.x0 + e->terreno.x1) / 2;
  e->cant_tesoros = MAX_TESOROS;
  for (int i = 0; i < e->cant_tesoros; i++) {
    e->tesoros[i].x = desde_x + rand() % (e->terreno.x1 - desde_x + 1);
    e->tesoros[i].y =
        e->terreno.y0 + rand() % (e->terreno.y1 - e->terreno.y0 + 1);
    e->tesoros[i].tipo = rand() % REC_CANTIDAD;
    e->tesoros[i].cantidad = 50 + rand() % 151; // entre 50 y 200
    e->tesoros[i].tomado = 0;
  }

  e->seleccion.activa = 0;
  e->inicializado = 1;
}

// --- Cuarteles ---------------------------------------------------------------

static void colocar_cuartel(Estado *e, int x, int y) {
  if (e->cant_cuarteles >= MAX_CUARTELES)
    return;

  terreno_recortar(&e->terreno, &x, &y);

  Cuartel *c = &e->cuarteles[e->cant_cuarteles++];
  c->x = x;
  c->y = y;
  c->activo = 1;
  c->entrenados = 0;
  // El primero sale enseguida; el resto respeta la cadencia. Si esperara la
  // cadencia completa, plantar el cuartel se sentiria muerto.
  c->entrena_espera = 1.0f;
}

// Busca donde meter un soldado nuevo: primero la ranura de un caido, y si no
// hay, el final. Devuelve -1 si ya estan los MAX_SOLDADOS vivos.
//
// Reusar las ranuras es lo que permite reponer bajas entre oleada y oleada. Sin
// esto, cant_soldados solo subiria, tocaria el tope con los muertos incluidos y
// los cuarteles no producirian nunca mas.
static int ranura_soldado(Estado *e) {
  // El techo de poblacion se chequea ANTES de buscar lugar: aunque haya una
  // ranura de un caido libre, si la poblacion esta al tope no entra nadie.
  if (e->recursos.poblacion >= POBLACION_MAX)
    return -1;

  for (int i = 0; i < e->cant_soldados; i++)
    if (!e->soldados[i].vivo)
      return i;

  if (e->cant_soldados < MAX_SOLDADOS)
    return e->cant_soldados++;

  return -1;
}

// La poblacion no se lleva sumando y restando a mano en cada alta y cada baja:
// se recalcula contando lo que hay vivo. Un contador incremental se
// desincroniza en cuanto te olvidas de restar en un camino de muerte, y el bug
// aparece recien horas despues.
static void recalcular_poblacion(Estado *e) {
  int vivos = 0;
  for (int i = 0; i < e->cant_soldados; i++)
    vivos += e->soldados[i].vivo;
  e->recursos.poblacion = vivos;
}

// Saca un soldado al lado del cuartel. La posicion rota con la cuenta de
// entrenados para que no se apilen todos en la misma celda al salir.
static void entrenar_soldado(Estado *e, Cuartel *c) {
  int i = ranura_soldado(e);
  if (i < 0)
    return; // tropa completa: el temporizador sigue, pero no produce

  int x = c->x - 2;
  int y = c->y + (c->entrenados % 3) - 1;
  terreno_recortar(&e->terreno, &x, &y);

  soldado_init(&e->soldados[i], (float)x, (float)y);
  c->entrenados++;
}

static void cuarteles_update(Estado *e, float dt) {
  for (int i = 0; i < e->cant_cuarteles; i++) {
    Cuartel *c = &e->cuarteles[i];
    if (!c->activo)
      continue;

    c->entrena_espera -= dt;
    if (c->entrena_espera <= 0.0f) {
      entrenar_soldado(e, c);
      c->entrena_espera = CUARTEL_ENTRENA_CADA;
    }
  }
}

// --- Aparicion de enemigos ---------------------------------------------------

// Busca un lugar libre en el arreglo: primero una ranura de un muerto, y si no
// hay, el final. Devuelve -1 si ya no entra ninguno mas.
static int ranura_enemigo(Estado *e) {
  for (int i = 0; i < e->cant_enemigos; i++)
    if (!e->enemigos[i].cuerpo.vivo)
      return i;

  if (e->cant_enemigos < MAX_ENEMIGOS)
    return e->cant_enemigos++;

  return -1;
}

static float dist2(float ax, float ay, float bx, float by) {
  float dx = bx - ax, dy = by - ay;
  return dx * dx + dy * dy;
}

// Cuantos guardias vivos tiene ese tesoro encima ahora mismo.
static int guardias_de(const Estado *e, const Tesoro *t) {
  int n = 0;
  for (int i = 0; i < e->cant_enemigos; i++) {
    if (!e->enemigos[i].cuerpo.vivo)
      continue;
    if (dist2(e->enemigos[i].puesto_x, e->enemigos[i].puesto_y, (float)t->x,
              (float)t->y) <= RADIO_CUSTODIA * RADIO_CUSTODIA)
      n++;
  }
  return n;
}

// Elige el tesoro sin tomar que menos guardias tenga, para que la defensa se
// reparta sola en vez de amontonarse toda en el primero.
static int tesoro_mas_flojo(const Estado *e) {
  int mejor = -1, mejor_n = 0;
  for (int i = 0; i < e->cant_tesoros; i++) {
    if (e->tesoros[i].tomado)
      continue;
    int n = guardias_de(e, &e->tesoros[i]);
    if (mejor < 0 || n < mejor_n) {
      mejor = i;
      mejor_n = n;
    }
  }
  return mejor;
}

static void aparecer_enemigo(Estado *e) {
  int i = ranura_enemigo(e);
  if (i < 0)
    return; // el campo esta lleno; se saltea este turno de aparicion

  // Justo adentro de la pared derecha, en una fila cualquiera de la apertura.
  int filas = e->terreno.apertura_y1 - e->terreno.apertura_y0 + 1;
  int y = e->terreno.apertura_y0 + rand() % filas;
  enemigo_init(&e->enemigos[i], (float)e->terreno.x1, (float)y);

  // Entra por la apertura pero no viene por vos: se va derecho a custodiar un
  // tesoro. Los cuatro puestos alrededor evitan que los guardias de un mismo
  // tesoro queden pisados en la misma celda.
  int t = tesoro_mas_flojo(e);
  if (t < 0)
    return; // no queda nada que custodiar: se queda donde nacio

  static const int offx[4] = {-2, 2, 0, 0};
  static const int offy[4] = {0, 0, -1, 1};
  int ranura = guardias_de(e, &e->tesoros[t]) % 4;

  int px = e->tesoros[t].x + offx[ranura];
  int py = e->tesoros[t].y + offy[ranura];
  terreno_recortar(&e->terreno, &px, &py);
  enemigo_vigilar(&e->enemigos[i], (float)px, (float)py);
}

// Un tesoro se levanta cuando ya no le queda un guardia vivo y un soldado tuyo
// esta encima. Los dos chequeos importan: sin el primero alcanzaria con correr
// a tocarlo esquivando la defensa.
static void recoger_tesoros(Estado *e) {
  for (int i = 0; i < e->cant_tesoros; i++) {
    Tesoro *t = &e->tesoros[i];
    if (t->tomado)
      continue;
    if (guardias_de(e, t) > 0)
      continue;

    for (int j = 0; j < e->cant_soldados; j++) {
      if (!e->soldados[j].vivo)
        continue;
      if (dist2(e->soldados[j].x, e->soldados[j].y, (float)t->x, (float)t->y) >
          RADIO_RECOGIDA * RADIO_RECOGIDA)
        continue;

      e->recursos.cantidad[t->tipo] += t->cantidad;
      t->tomado = 1;
      break;
    }
  }
}

// --- game_update -------------------------------------------------------------

// Marca como seleccionado todo lo VIVO que caiga adentro del recuadro, de los
// dos bandos. Empieza limpiando: una seleccion nueva reemplaza a la anterior,
// no se suma.
//
// Los enemigos se seleccionan para MIRAR, no para mandar: seleccionar y ordenar
// son dos cosas distintas, y quien filtra por bando es ordenar_movimiento(),
// que solo recorre e->soldados. Un enemigo seleccionado muestra su vida y nada
// mas.
static void aplicar_seleccion(Estado *e) {
  for (int i = 0; i < e->cant_soldados; i++)
    e->soldados[i].seleccionado =
        e->soldados[i].vivo &&
        dentro_del_recuadro(&e->seleccion, e->cam_x, e->cam_y,
                            e->soldados[i].x, e->soldados[i].y);

  for (int i = 0; i < e->cant_enemigos; i++) {
    Soldado *c = &e->enemigos[i].cuerpo;
    c->seleccionado = c->vivo && dentro_del_recuadro(&e->seleccion, e->cam_x,
                                                     e->cam_y, c->x, c->y);
  }
}

// Manda a los seleccionados hacia (cx, cy), cada uno a su lugar en la rejilla.
static void ordenar_movimiento(Estado *e, int cx, int cy) {
  // Primero hay que saber cuantos van, porque la forma de la rejilla depende
  // del total. Por eso son dos recorridas y no una.
  int total = 0;
  for (int i = 0; i < e->cant_soldados; i++)
    if (e->soldados[i].seleccionado && e->soldados[i].vivo)
      total++;

  if (total == 0)
    return;

  int slot = 0;
  for (int i = 0; i < e->cant_soldados; i++) {
    if (!e->soldados[i].seleccionado || !e->soldados[i].vivo)
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

  // --- Teclado ---
  // Va antes que el mouse para que apretar 'b' y clickear en el mismo frame
  // funcione. El movimiento por teclas quedo en tecla_a_direccion()
  // (playerC.c), compilada y lista, pero hoy no se llama.
  for (int i = 0; i < in.n; i++) {
    if (in.bytes[i] == 'q')
      return 0; // devolver 0 corta el bucle del host y sale
    if (in.bytes[i] == 'b')
      e->modo_construir = !e->modo_construir; // vuelve a apretar y lo cancelas
  }

  // --- Boton IZQUIERDO: seleccionar, y nada mas ---
  // Es el selector de unidades y no comparte el boton con ninguna otra cosa.
  // Un click seco es un recuadro de una sola celda, asi que el mismo camino
  // resuelve el click puntual y el arrastre.
  if (in.mouse.izq_apretado && en_el_mapa(e, in.mouse.y)) {
    if (e->modo_construir) {
      int wx, wy;
      a_mundo(e, in.mouse.x, in.mouse.y, &wx, &wy);
      colocar_cuartel(e, wx, wy);
      e->modo_construir = 0;
    } else {
      e->seleccion.activa = 1;
      e->seleccion.x0 = e->seleccion.x1 = in.mouse.x;
      e->seleccion.y0 = e->seleccion.y1 = in.mouse.y;
    }
  }

  if (e->seleccion.activa && in.mouse.arrastrando) {
    e->seleccion.x1 = in.mouse.x;
    e->seleccion.y1 = in.mouse.y;
  }

  if (in.mouse.izq_soltado && e->seleccion.activa) {
    e->seleccion.x1 = in.mouse.x;
    e->seleccion.y1 = in.mouse.y;
    aplicar_seleccion(e);
    e->seleccion.activa = 0;
  }

  // --- Boton DERECHO: mover la camara o dar la orden ---
  // Los dos van en el mismo boton, y lo que los separa es si arrastraste:
  //
  //   arrastrar -> mueve la camara
  //   click seco -> manda a los seleccionados a ese punto
  //
  // Se decide al SOLTAR, no al apretar: cuando apretas todavia no se sabe si
  // vas a arrastrar. Por eso la orden sale de der_soltado y no de der_apretado.
  if (in.mouse.der_apretado && en_el_mapa(e, in.mouse.y)) {
    e->pan_activo = 1;
    e->hubo_arrastre = 0;
    e->pan_x = in.mouse.x;
    e->pan_y = in.mouse.y;
  }

  if (e->pan_activo && in.mouse.der_arrastrando) {
    e->hubo_arrastre = 1;
    // La camara se mueve al REVES que el cursor: estas agarrando el mapa y
    // tirando de el, no empujando una ventana.
    e->cam_x -= in.mouse.x - e->pan_x;
    e->cam_y -= in.mouse.y - e->pan_y;
    e->pan_x = in.mouse.x;
    e->pan_y = in.mouse.y;
    camara_recortar(e);
  }

  if (in.mouse.der_soltado) {
    if (e->pan_activo && !e->hubo_arrastre && en_el_mapa(e, in.mouse.y)) {
      int x, y;
      a_mundo(e, in.mouse.x, in.mouse.y, &x, &y);
      terreno_recortar(&e->terreno, &x, &y);
      ordenar_movimiento(e, x, y);
    }
    e->pan_activo = 0;
  }

  // --- Cuarteles ---
  // La poblacion se recalcula ANTES de entrenar, para que el cuartel decida
  // con el numero de este frame y no con el del anterior.
  recalcular_poblacion(e);
  cuarteles_update(e, dt);

  // --- Ciclo de oleadas ---
  // Mientras corre la preparacion no entra nadie. El contador se frena en 0 y
  // no sigue a negativo: se lo muestra en pantalla y ademas es la condicion que
  // habilita la invasion.
  if (e->preparacion > 0.0f) {
    e->preparacion -= dt;
    if (e->preparacion < 0.0f)
      e->preparacion = 0.0f;
  } else if (e->por_aparecer > 0) {
    e->spawn_espera -= dt;
    if (e->spawn_espera <= 0.0f) {
      aparecer_enemigo(e);
      e->por_aparecer--;
      e->spawn_espera = SPAWN_CADA;
    }
  } else {
    // Ya entraron todos los de la oleada. La invasion no termina cuando dejan
    // de aparecer, sino cuando no queda ninguno vivo: si no, el contador
    // volveria con enemigos todavia en el campo.
    int quedan = 0;
    for (int i = 0; i < e->cant_enemigos; i++)
      quedan += e->enemigos[i].cuerpo.vivo;

    if (quedan == 0) {
      e->oleada++;
      e->por_aparecer = ENEMIGOS_OLEADA_BASE + e->oleada;
      e->preparacion = PREPARACION_SIGUIENTE;
      e->spawn_espera = 1.0f;
    }
  }

  // --- Pelea ---
  // Los enemigos deciden primero (se acercan y disparan) y despues los soldados
  // devuelven. Los impactos se resuelven al final, sobre las balas que ya
  // estaban viajando: asi ninguna nace y pega en el mismo frame.
  combate_enemigos(e->enemigos, e->cant_enemigos, e->soldados, e->cant_soldados,
                   e->proyectiles, MAX_PROYECTILES, dt);
  combate_soldados(e->soldados, e->cant_soldados, e->enemigos, e->cant_enemigos,
                   e->proyectiles, MAX_PROYECTILES, dt);

  proyectiles_update(e->proyectiles, MAX_PROYECTILES, dt, e->terreno.x0,
                     e->terreno.y0, e->terreno.x1, e->terreno.y1);
  combate_impactos(e->proyectiles, MAX_PROYECTILES, e->soldados,
                   e->cant_soldados, e->enemigos, e->cant_enemigos);

  // Y otra vez al final, porque los impactos de este frame pudieron matar a
  // alguien y el panel tiene que mostrar el numero de ahora.
  recalcular_poblacion(e);

  // Despues de los impactos: si el ultimo guardia cayo en este frame, el tesoro
  // ya se puede levantar sin esperar al siguiente.
  recoger_tesoros(e);

  // --- Movimiento ---
  for (int i = 0; i < e->cant_soldados; i++)
    if (e->soldados[i].vivo)
      soldado_update(&e->soldados[i], dt);

  for (int i = 0; i < e->cant_enemigos; i++)
    if (e->enemigos[i].cuerpo.vivo)
      soldado_update(&e->enemigos[i].cuerpo, dt);

  return 1;
}

// --- game_render -------------------------------------------------------------

// Dibuja una celda dada en coordenadas de MUNDO. Restarle la camara la lleva a
// la pantalla; fb_poner recorta lo que se va del framebuffer, y el chequeo
// contra piso_alto evita que el mapa invada el panel de abajo.
//
// Esta funcion es la UNICA que sabe que existe una camara. Todo el resto del
// render habla en coordenadas de mundo y no se entera.
static void poner_mundo(Framebuffer *fb, const Estado *e, int wx, int wy,
                        char ch, unsigned char color) {
  int sy = wy - e->cam_y;
  if (sy < 0 || sy >= e->piso_alto)
    return;
  fb_poner(fb, wx - e->cam_x, sy, ch, color);
}

// La pared se saltea en las filas de la apertura: ese hueco ES la puerta.
//
// Se recorre solo el tramo de pared que puede llegar a verse. Sin ese recorte
// se recorrerian las 240 columnas del mundo en cada frame para descartar casi
// todas: funciona igual, pero es trabajo tirado.
static void dibujar_paredes(Framebuffer *fb, const Estado *e) {
  const Terreno *t = &e->terreno;

  int desde_x = e->cam_x - 1, hasta_x = e->cam_x + e->ancho;
  if (desde_x < t->x0 - 1)
    desde_x = t->x0 - 1;
  if (hasta_x > t->x1 + 1)
    hasta_x = t->x1 + 1;

  for (int x = desde_x; x <= hasta_x; x++) {
    poner_mundo(fb, e, x, t->y0 - 1, '#', NARANJA);
    poner_mundo(fb, e, x, t->y1 + 1, '#', NARANJA);
  }

  int desde_y = e->cam_y - 1, hasta_y = e->cam_y + e->piso_alto;
  if (desde_y < t->y0 - 1)
    desde_y = t->y0 - 1;
  if (hasta_y > t->y1 + 1)
    hasta_y = t->y1 + 1;

  for (int y = desde_y; y <= hasta_y; y++) {
    poner_mundo(fb, e, t->x0 - 1, y, '#', NARANJA);

    // El hueco va en rojo y no en naranja: tiene que gritar que por ahi entran,
    // no confundirse con un pedazo mas de pared.
    if (y >= t->apertura_y0 && y <= t->apertura_y1)
      poner_mundo(fb, e, t->x1 + 1, y, '.', 31);
    else
      poner_mundo(fb, e, t->x1 + 1, y, '#', NARANJA);
  }
}

// Verde arriba del 60%, amarillo hasta el 30%, rojo abajo de eso.
static unsigned char color_por_vida(int hp, int max_hp) {
  if (max_hp <= 0)
    return 32;
  int pct = hp * 100 / max_hp;
  if (pct > 60)
    return 32; // verde
  if (pct > 30)
    return 33; // amarillo
  return 31;   // rojo
}

// Barra de BARRA_ANCHO celdas justo encima de la unidad.
//
// Se dibuja SOLO para las unidades seleccionadas: con seis soldados y hasta
// veinticuatro enemigos, una barra sobre cada uno tapa el campo y no se
// distingue nada. La vida del resto igual se lee, porque el char de cada unidad
// se tinie con el mismo color_por_vida().
//
// Los caracteres son finos a proposito ('-' y '.' en vez de '=' y '-'): la
// barra tiene que leerse de un vistazo sin competir con la unidad que esta
// justo abajo.
static void dibujar_barra(Framebuffer *fb, const Estado *e, int x, int y,
                          int hp, int max_hp) {
  if (max_hp <= 0)
    return;

  // La multiplicacion va ANTES de la division a proposito. Con enteros,
  // (hp / max_hp) * BARRA_ANCHO daria 0 siempre, porque hp/max_hp se trunca a 0
  // para cualquier vida que no sea la maxima.
  int llenas = hp * BARRA_ANCHO / max_hp;

  // Mientras quede un solo punto de vida, que se vea al menos una celda: si no,
  // una unidad moribunda parece muerta.
  if (llenas == 0 && hp > 0)
    llenas = 1;

  unsigned char color = color_por_vida(hp, max_hp);
  int inicio = x - BARRA_ANCHO / 2;

  for (int i = 0; i < BARRA_ANCHO; i++)
    poner_mundo(fb, e, inicio + i, y - 1, i < llenas ? '-' : '.',
                i < llenas ? color : 90);
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

// --- Panel de abajo ----------------------------------------------------------

// Caja de bordes ASCII con el titulo incrustado en el borde de arriba.
static void dibujar_caja(Framebuffer *fb, int x0, int y0, int x1, int y1,
                         const char *titulo, unsigned char color) {
  for (int x = x0; x <= x1; x++) {
    fb_poner(fb, x, y0, '-', color);
    fb_poner(fb, x, y1, '-', color);
  }
  for (int y = y0; y <= y1; y++) {
    fb_poner(fb, x0, y, '|', color);
    fb_poner(fb, x1, y, '|', color);
  }
  fb_poner(fb, x0, y0, '+', color);
  fb_poner(fb, x1, y0, '+', color);
  fb_poner(fb, x0, y1, '+', color);
  fb_poner(fb, x1, y1, '+', color);

  fb_texto(fb, x0 + 2, y0, titulo, color);
}

// Caja izquierda: lo que tenes. Los ocho recursos en rejilla de tres columnas,
// y la poblacion abajo con su techo.
static void dibujar_caja_recursos(Framebuffer *fb, const Recursos *r, int x0,
                                  int y0, int x1, int y1) {
  dibujar_caja(fb, x0, y0, x1, y1, " RECURSOS ", 33);

  // Cuatro columnas y no tres: con diez recursos, tres columnas necesitan
  // cuatro filas y la ultima le pisaria el lugar a la poblacion.
  const int COLUMNAS = 4;
  const int ANCHO_COL = 12;

  for (int i = 0; i < REC_CANTIDAD; i++) {
    int col = i % COLUMNAS;
    int fila = i / COLUMNAS;

    char linea[32];
    snprintf(linea, sizeof linea, "%-7s%4d", RECURSO_NOMBRE[i],
             r->cantidad[i]);
    fb_texto(fb, x0 + 2 + col * ANCHO_COL, y0 + 1 + fila, linea, 37);
  }

  // La poblacion se pinta roja cuando llego al techo: es el unico numero del
  // panel que te bloquea algo si lo tocas.
  char pob[32];
  snprintf(pob, sizeof pob, "POBLACION %d/%d", r->poblacion, POBLACION_MAX);
  fb_texto(fb, x0 + 2, y1 - 1, pob,
           r->poblacion >= POBLACION_MAX ? 31 : 32);
}

// Caja derecha: lo que hay dado vuelta en el mapa ahora mismo.
static void dibujar_caja_mapa(Framebuffer *fb, const Estado *e, int x0, int y0,
                              int x1, int y1) {
  dibujar_caja(fb, x0, y0, x1, y1, " EN EL MAPA ", 36);

  int soldados = 0, enemigos = 0, tesoros = 0;
  for (int i = 0; i < e->cant_soldados; i++)
    soldados += e->soldados[i].vivo;
  for (int i = 0; i < e->cant_enemigos; i++)
    enemigos += e->enemigos[i].cuerpo.vivo;
  for (int i = 0; i < e->cant_tesoros; i++)
    tesoros += !e->tesoros[i].tomado;

  char l[48];
  snprintf(l, sizeof l, "SOLD %d/%d", soldados, MAX_SOLDADOS);
  fb_texto(fb, x0 + 2, y0 + 1, l, 36);
  snprintf(l, sizeof l, "CUART %d/%d", e->cant_cuarteles, MAX_CUARTELES);
  fb_texto(fb, x0 + 11, y0 + 1, l, 35);

  snprintf(l, sizeof l, "ENEM %d", enemigos);
  fb_texto(fb, x0 + 2, y0 + 2, l, 31);
  snprintf(l, sizeof l, "TESOROS %d", tesoros);
  fb_texto(fb, x0 + 11, y0 + 2, l, 33);

  if (e->preparacion > 0.0f)
    snprintf(l, sizeof l, "OLEADA %d  PAZ %ds", e->oleada + 1,
             (int)(e->preparacion + 0.999f));
  else
    snprintf(l, sizeof l, "OLEADA %d  ENTRAN %d", e->oleada + 1,
             e->por_aparecer);
  fb_texto(fb, x0 + 2, y0 + 3, l, 37);

  // El aviso de construccion vivia arriba del mapa. Ahora que arriba no hay
  // nada, baja aca: es el unico texto que cambia segun lo que estas haciendo.
  fb_texto(fb, x0 + 2, y1 - 1,
           e->modo_construir ? "CLICK: PLANTAR CUARTEL" : "b=cuartel  q=salir",
           e->modo_construir ? 35 : 90);
}

// Caja derecha: el mapa entero achicado, al estilo del minimapa de AoE.
//
// La escala se calcula desde el tamanio del terreno, no con numeros a mano: si
// maniana cambias FB_ANCHO o el margen, el minimapa se reacomoda solo.
static void dibujar_minimapa(Framebuffer *fb, const Estado *e, int x0, int y0,
                             int x1, int y1) {
  dibujar_caja(fb, x0, y0, x1, y1, " MINIMAPA ", NARANJA);

  int disp_ancho = x1 - x0 - 1; // area interior, sin los bordes
  int disp_alto = y1 - y0 - 1;
  if (disp_ancho <= 0 || disp_alto <= 0)
    return;

  int mapa_ancho = e->terreno.x1 - e->terreno.x0 + 1;
  int mapa_alto = e->terreno.y1 - e->terreno.y0 + 1;

  // El minimapa NO se estira para llenar la caja: conserva la forma del mundo.
  //
  // Una celda de terminal es como el doble de alta que de ancha, asi que para
  // que un mundo de 240x80 no se vea achatado hacen falta 240/(80*2) = 1.5
  // columnas por cada fila-visual. Se toma la dimension que primero se llena y
  // la otra sale de ahi.
  int alto = disp_alto;
  int ancho = alto * 2 * mapa_ancho / mapa_alto;
  if (ancho > disp_ancho) {
    ancho = disp_ancho;
    alto = ancho * mapa_alto / (2 * mapa_ancho);
    if (alto < 1)
      alto = 1;
  }

  // Centrado en la caja, para que el aire sobrante quede repartido.
  int ix0 = x0 + 1 + (disp_ancho - ancho) / 2;
  int iy0 = y0 + 1 + (disp_alto - alto) / 2;

  // Lleva una coordenada del mundo a una celda del minimapa.
#define MINI_X(mx) (ix0 + ((mx) - e->terreno.x0) * ancho / mapa_ancho)
#define MINI_Y(my) (iy0 + ((my) - e->terreno.y0) * alto / mapa_alto)

  // El recuadro de lo que estas viendo en el mapa grande. Va primero, abajo de
  // todo: es contexto, no tiene que taparte una unidad.
  int vx0 = MINI_X(e->cam_x);
  int vy0 = MINI_Y(e->cam_y);
  int vx1 = MINI_X(e->cam_x + e->ancho - 1);
  int vy1 = MINI_Y(e->cam_y + e->piso_alto - 1);
  for (int x = vx0; x <= vx1; x++) {
    fb_poner(fb, x, vy0, '-', 37);
    fb_poner(fb, x, vy1, '-', 37);
  }
  for (int y = vy0; y <= vy1; y++) {
    fb_poner(fb, vx0, y, '|', 37);
    fb_poner(fb, vx1, y, '|', 37);
  }

  // Se dibuja de menos a mas importante: lo que viene despues pisa a lo
  // anterior. En un minimapa donde una celda son varias del mapa, ese orden ES
  // la regla de prioridad.
  for (int i = 0; i < e->cant_tesoros; i++) {
    if (e->tesoros[i].tomado)
      continue;
    fb_poner(fb, MINI_X(e->tesoros[i].x), MINI_Y(e->tesoros[i].y), '$', 33);
  }

  for (int i = 0; i < e->cant_cuarteles; i++) {
    if (!e->cuarteles[i].activo)
      continue;
    fb_poner(fb, MINI_X(e->cuarteles[i].x), MINI_Y(e->cuarteles[i].y), 'B', 35);
  }

  for (int i = 0; i < e->cant_enemigos; i++) {
    if (!e->enemigos[i].cuerpo.vivo)
      continue;
    fb_poner(fb, MINI_X((int)e->enemigos[i].cuerpo.x),
             MINI_Y((int)e->enemigos[i].cuerpo.y), 'x', 31);
  }

  for (int i = 0; i < e->cant_soldados; i++) {
    if (!e->soldados[i].vivo)
      continue;
    fb_poner(fb, MINI_X((int)e->soldados[i].x), MINI_Y((int)e->soldados[i].y),
             'o', 36);
  }

#undef MINI_X
#undef MINI_Y
}

void game_render(void *mem, size_t size, Framebuffer *fb) {
  if (size < sizeof(Estado))
    return;

  Estado *e = (Estado *)mem;
  fb_limpiar(fb);

  dibujar_paredes(fb, e);

  // Tesoros primero: van abajo de todo para que un guardia parado encima se vea
  // igual. El '$' se descubre solo cuando limpiaste la defensa.
  for (int i = 0; i < e->cant_tesoros; i++) {
    if (e->tesoros[i].tomado)
      continue;
    poner_mundo(fb, e, e->tesoros[i].x, e->tesoros[i].y, '$', 33);
  }

  // Marcas de destino primero, para que ninguna unidad quede tapada por una.
  for (int i = 0; i < e->cant_soldados; i++)
    if (e->soldados[i].vivo && e->soldados[i].moviendose)
      poner_mundo(fb, e, (int)e->soldados[i].destino_x,
                  (int)e->soldados[i].destino_y, 'x', 33);

  // Enemigos. Muestran la vida con la misma regla que los soldados: solo si
  // estan seleccionados. El char no cambia al seleccionarlos (los soldados
  // pasan de @ a O) porque un enemigo no obedece ordenes: la barra alcanza como
  // seniial de que lo estas mirando.
  for (int i = 0; i < e->cant_enemigos; i++) {
    const Soldado *c = &e->enemigos[i].cuerpo;
    if (!c->vivo)
      continue;
    if (c->seleccionado)
      dibujar_barra(fb, e, (int)c->x, (int)c->y, c->hp, c->max_hp);
    poner_mundo(fb, e, (int)c->x, (int)c->y, 'M',
                color_por_vida(c->hp, c->max_hp));
  }

  // La posicion es float y el framebuffer trabaja en celdas enteras: el casteo
  // a int es el unico lugar donde se pierden los decimales, y esta bien que sea
  // asi. Los decimales siguen acumulandose en el Estado.
  for (int i = 0; i < e->cant_soldados; i++) {
    const Soldado *s = &e->soldados[i];
    if (!s->vivo)
      continue;
    if (s->seleccionado)
      dibujar_barra(fb, e, (int)s->x, (int)s->y, s->hp, s->max_hp);
    poner_mundo(fb, e, (int)s->x, (int)s->y, s->seleccionado ? 'O' : '@',
                color_por_vida(s->hp, s->max_hp));
  }

  // Balas. Las tuyas son rayitas cian, las del enemigo puntos rojos: de un
  // vistazo tenes que saber en que direccion va el tiroteo.
  for (int i = 0; i < MAX_PROYECTILES; i++) {
    const Proyectil *p = &e->proyectiles[i];
    if (!p->activo)
      continue;
    poner_mundo(fb, e, (int)p->x, (int)p->y,
                p->bando == BANDO_SOLDADO ? '-' : '.',
                p->bando == BANDO_SOLDADO ? 36 : 31);
  }

  // Cuarteles al final del dibujo del campo, para que ninguna unidad los tape.
  for (int i = 0; i < e->cant_cuarteles; i++) {
    if (!e->cuarteles[i].activo)
      continue;
    poner_mundo(fb, e, e->cuarteles[i].x, e->cuarteles[i].y, 'B', 35);
  }

  // El recuadro va ultimo para que se vea por encima de todo mientras arrastras.
  if (e->seleccion.activa)
    dibujar_recuadro(fb, &e->seleccion);

  // --- Panel de abajo, dos cajas lado a lado ---
  // Arranca justo donde termina el mapa. Las dos ocupan las mismas filas para
  // que el borde de abajo quede alineado.
  int panel_y0 = e->piso_alto;
  int panel_y1 = e->piso_alto + PANEL_ALTO - 1;

  // Las dos primeras cajas tienen ancho fijo porque su contenido tambien lo
  // tiene: los diez recursos en cuatro columnas necesitan 52 celdas, y no se
  // achican sin romper la grilla. El minimapa se queda con lo que sobre, que es
  // justo lo que un minimapa puede hacer: reescalar.
  const int REC_ANCHO = 52;
  const int MAPA_ANCHO = 23;

  dibujar_caja_recursos(fb, &e->recursos, 0, panel_y0, REC_ANCHO - 1, panel_y1);

  int mapa_x0 = REC_ANCHO + 1;
  if (mapa_x0 + MAPA_ANCHO > e->ancho)
    return; // terminal angosta: con los recursos alcanza
  dibujar_caja_mapa(fb, e, mapa_x0, panel_y0, mapa_x0 + MAPA_ANCHO - 1,
                    panel_y1);

  // El minimapa tiene el MISMO ancho fijo que la caja del medio y no se estira
  // con la terminal. Una caja de minimapa larguisima no muestra mas mundo, solo
  // deforma el que ya muestra.
  int mini_x0 = mapa_x0 + MAPA_ANCHO + 1;
  if (mini_x0 + MAPA_ANCHO > e->ancho)
    return; // no entra entera; mejor no dibujarla cortada
  dibujar_minimapa(fb, e, mini_x0, panel_y0, mini_x0 + MAPA_ANCHO - 1,
                   panel_y1);
}
