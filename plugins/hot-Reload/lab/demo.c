// demo.c — modulo minimo que SI implementa el contrato de platform.h.
//
// Esto NO es tu juego: existe para que veas el hot-reload funcionando de punta
// a punta. Un `@` en una sala, enemigos que entran por las cuatro paredes, vida
// y un golpe cuerpo a cuerpo.
//
// Controles: wasd = mover, f = golpear, r = reiniciar, q = salir.
//
// Probalo asi: con el host corriendo en otro panel, cambiale ENEMIGO_DANIO o
// ALCANCE_GOLPE y guarda. El juego cambia sin perder la partida.

#include "platform.h"
#include <stdio.h>
#include <time.h>

// --- Ajustes ---
// Todo lo que vale la pena tocar en caliente esta aca arriba. Cambiar
// cualquiera de estos numeros y guardar es el ciclo que queres sentir.

#define SALA_ANCHO 40
#define SALA_ALTO 16

#define MAX_ENEMIGOS 24
#define META_MUERTES 10

#define JUGADOR_HP 100
#define ENEMIGO_DANIO 7
#define ALCANCE_GOLPE 1 // en celdas, alrededor del jugador

#define SPAWN_INICIAL 1.30f    // segundos entre apariciones al empezar
#define SPAWN_MINIMO 0.35f     // piso: mas rapido que esto no aparecen
#define SPAWN_POR_MUERTE 0.09f // cuanto se acelera con cada enemigo muerto
#define ENEMIGO_PERIODO 0.28f  // segundos entre pasos de los enemigos
#define GOLPE_FRAMES 4         // cuantos frames se ve el destello del golpe

// --- Estado ---
// Todo tu estado vive en un struct, y ese struct vive dentro del bloque `mem`
// que te presta el host. El host hizo el calloc una sola vez y no lo suelta
// nunca: por eso sobrevive las recargas.
//
// Este numero es la version del layout. Si le cambias la forma al struct, los
// bytes viejos que quedaron en `mem` ya no significan lo que este codigo cree,
// asi que subilo y game_update reinicializa en vez de leer basura.
#define ESTADO_MAGIC 0x5AFE0002u

typedef enum { JUGANDO, GANASTE, PERDISTE } Fase;

typedef struct {
  int x, y;
  int vivo;
} Enemigo;

typedef struct {
  unsigned magic;
  Fase fase;

  int x, y;
  int hp;
  int muertos;

  Enemigo enemigos[MAX_ENEMIGOS];

  float t_spawn;    // acumulador para la proxima aparicion
  float t_enemigos; // acumulador para el proximo paso de los enemigos
  int golpe_frames; // frames que le quedan al destello del golpe

  unsigned azar; // semilla del generador; ver siguiente_azar
} Estado;

// El host te pasa un void*. Vos lo reinterpretas como tu struct. El host no
// sabe ni le importa que forma tiene: por eso podes rediseniar Estado sin
// recompilar el host.
static Estado *estado(void *mem) { return (Estado *)mem; }

// --- Azar ---
// xorshift32. Alcanza y sobra, y lo importante: la semilla vive en TU estado,
// no adentro de libc. Asi la secuencia tambien sobrevive las recargas.

static unsigned siguiente_azar(Estado *e) {
  e->azar ^= e->azar << 13;
  e->azar ^= e->azar >> 17;
  e->azar ^= e->azar << 5;
  return e->azar;
}

static int azar_hasta(Estado *e, int n) {
  return (int)(siguiente_azar(e) % (unsigned)n);
}

// --- Consultas ---

static int es_pared(int x, int y) {
  return x <= 0 || x >= SALA_ANCHO - 1 || y <= 0 || y >= SALA_ALTO - 1;
}

static Enemigo *enemigo_en(Estado *e, int x, int y) {
  for (int i = 0; i < MAX_ENEMIGOS; i++)
    if (e->enemigos[i].vivo && e->enemigos[i].x == x && e->enemigos[i].y == y)
      return &e->enemigos[i];
  return NULL;
}

// Devuelve -1, 0 o 1: en que direccion hay que moverse para acercarse.
static int signo(int desde, int hasta) {
  if (hasta > desde)
    return 1;
  if (hasta < desde)
    return -1;
  return 0;
}

static int valor_absoluto(int n) { return n < 0 ? -n : n; }

// --- Enemigos ---

// Aparecen pegados a una de las cuatro paredes, elegida al azar.
static void aparecer_enemigo(Estado *e) {
  int libre = -1;
  for (int i = 0; i < MAX_ENEMIGOS; i++) {
    if (!e->enemigos[i].vivo) {
      libre = i;
      break;
    }
  }
  if (libre < 0)
    return; // la sala esta llena, este turno no aparece nadie

  int x = 0, y = 0;
  switch (azar_hasta(e, 4)) {
  case 0: // pared de arriba
    x = 1 + azar_hasta(e, SALA_ANCHO - 2);
    y = 1;
    break;
  case 1: // pared de abajo
    x = 1 + azar_hasta(e, SALA_ANCHO - 2);
    y = SALA_ALTO - 2;
    break;
  case 2: // pared izquierda
    x = 1;
    y = 1 + azar_hasta(e, SALA_ALTO - 2);
    break;
  default: // pared derecha
    x = SALA_ANCHO - 2;
    y = 1 + azar_hasta(e, SALA_ALTO - 2);
    break;
  }

  // No aparecer encima del jugador ni encima de otro enemigo.
  if ((x == e->x && y == e->y) || enemigo_en(e, x, y))
    return;

  e->enemigos[libre].x = x;
  e->enemigos[libre].y = y;
  e->enemigos[libre].vivo = 1;
}

// Un paso de cada enemigo hacia el jugador. Si el paso lo llevaria justo a la
// celda del jugador, en vez de moverse pega.
static void mover_enemigos(Estado *e) {
  for (int i = 0; i < MAX_ENEMIGOS; i++) {
    Enemigo *en = &e->enemigos[i];
    if (!en->vivo)
      continue;

    int dx = signo(en->x, e->x);
    int dy = signo(en->y, e->y);

    // Se avanza primero por el eje donde falta mas: da una persecucion mas
    // natural que moverse siempre en diagonal.
    int primero_en_x =
        valor_absoluto(e->x - en->x) >= valor_absoluto(e->y - en->y);
    int pasos[2][2] = {{primero_en_x ? dx : 0, primero_en_x ? 0 : dy},
                       {primero_en_x ? 0 : dx, primero_en_x ? dy : 0}};

    for (int p = 0; p < 2; p++) {
      int nx = en->x + pasos[p][0];
      int ny = en->y + pasos[p][1];

      if (nx == en->x && ny == en->y)
        continue; // ese eje ya esta alineado, no hay paso que dar
      if (es_pared(nx, ny))
        continue;

      if (nx == e->x && ny == e->y) {
        e->hp -= ENEMIGO_DANIO;
        break; // pego y se queda donde estaba
      }
      if (enemigo_en(e, nx, ny))
        continue; // ocupado por un companiero, probamos el otro eje

      en->x = nx;
      en->y = ny;
      break;
    }
  }
}

// --- Jugador ---

static void mover_jugador(Estado *e, int dx, int dy) {
  int nx = e->x + dx;
  int ny = e->y + dy;

  if (es_pared(nx, ny))
    return;
  if (enemigo_en(e, nx, ny))
    return; // los enemigos bloquean: para pasar hay que matarlos

  e->x = nx;
  e->y = ny;
}

// Mata a todo enemigo dentro del alcance, en las ocho direcciones.
static void golpear(Estado *e) {
  e->golpe_frames = GOLPE_FRAMES;

  for (int i = 0; i < MAX_ENEMIGOS; i++) {
    Enemigo *en = &e->enemigos[i];
    if (!en->vivo)
      continue;

    if (valor_absoluto(en->x - e->x) <= ALCANCE_GOLPE &&
        valor_absoluto(en->y - e->y) <= ALCANCE_GOLPE) {
      en->vivo = 0;
      e->muertos++;
    }
  }
}

// --- Contrato con el host ---

// Corre una sola vez, antes del primer frame. En una recarga NO se vuelve a
// llamar, y justamente por eso no perdes la partida.
void game_init(void *mem, size_t size) {
  (void)size;
  Estado *e = estado(mem);

  for (int i = 0; i < MAX_ENEMIGOS; i++)
    e->enemigos[i].vivo = 0;

  e->fase = JUGANDO;
  e->x = SALA_ANCHO / 2;
  e->y = SALA_ALTO / 2;
  e->hp = JUGADOR_HP;
  e->muertos = 0;
  e->t_spawn = 0.0f;
  e->t_enemigos = 0.0f;
  e->golpe_frames = 0;
  e->azar = (unsigned)time(NULL) | 1u; // xorshift se muere si la semilla es 0
  e->magic = ESTADO_MAGIC;
}

int game_update(void *mem, size_t size, Input in, float dt) {
  Estado *e = estado(mem);

  // El bloque del host es enorme, pero mas vale chequearlo que pisar memoria
  // que no es nuestra.
  if (sizeof(Estado) > size)
    return 0;

  // Migracion de estado: si el struct cambio de forma entre recargas, los
  // bytes que hay en `mem` ya no significan lo que este codigo cree. El magic
  // no coincide y arrancamos de nuevo, en vez de leer basura.
  if (e->magic != ESTADO_MAGIC)
    game_init(mem, size);

  if (e->golpe_frames > 0)
    e->golpe_frames--;

  // in.n puede ser 0: significa que el jugador no toco nada en este frame.
  for (int i = 0; i < in.n; i++) {
    switch (in.bytes[i]) {
    case 'q':
      return 0; // devolver 0 le dice al host que corte el bucle
    case 'r':
      game_init(mem, size);
      continue;
    }

    if (e->fase != JUGANDO)
      continue; // partida terminada: solo r y q siguen andando

    switch (in.bytes[i]) {
    case 'w':
      mover_jugador(e, 0, -1);
      break;
    case 's':
      mover_jugador(e, 0, 1);
      break;
    case 'a':
      mover_jugador(e, -1, 0);
      break;
    case 'd':
      mover_jugador(e, 1, 0);
      break;
    case 'f':
    case 'F':
      golpear(e);
      break;
    }
  }

  if (e->fase != JUGANDO)
    return 1;

  // Los acumuladores usan dt, no frames: si maniana subis los FPS del host, el
  // juego sigue yendo a la misma velocidad.
  float periodo_spawn = SPAWN_INICIAL - SPAWN_POR_MUERTE * (float)e->muertos;
  if (periodo_spawn < SPAWN_MINIMO)
    periodo_spawn = SPAWN_MINIMO;

  e->t_spawn += dt;
  if (e->t_spawn >= periodo_spawn) {
    e->t_spawn = 0.0f;
    aparecer_enemigo(e);
  }

  e->t_enemigos += dt;
  if (e->t_enemigos >= ENEMIGO_PERIODO) {
    e->t_enemigos = 0.0f;
    mover_enemigos(e);
  }

  if (e->hp <= 0) {
    e->hp = 0;
    e->fase = PERDISTE;
  } else if (e->muertos >= META_MUERTES) {
    e->fase = GANASTE;
  }

  return 1;
}

// --- Dibujo ---

static int largo_de(const char *s) {
  int n = 0;
  while (s[n])
    n++;
  return n;
}

static void texto_centrado(Framebuffer *fb, int y, const char *s,
                           unsigned char color) {
  fb_texto(fb, (SALA_ANCHO - largo_de(s)) / 2, y, s, color);
}

static void dibujar_hud(Framebuffer *fb, const Estado *e) {
  // Barra de vida de diez tramos, redondeando para arriba: mientras te quede
  // aunque sea 1 de HP se ve un tramo lleno.
  char barra[11];
  int llenos = (e->hp * 10 + JUGADOR_HP - 1) / JUGADOR_HP;
  for (int i = 0; i < 10; i++)
    barra[i] = i < llenos ? '#' : '.';
  barra[10] = '\0';

  char linea[SALA_ANCHO + 32];
  snprintf(linea, sizeof linea, "HP [%s] %3d    enemigos muertos: %2d/%d",
           barra, e->hp, e->muertos, META_MUERTES);
  fb_texto(fb, 0, SALA_ALTO, linea, e->hp <= 30 ? 31 : 32);

  fb_texto(fb, 0, SALA_ALTO + 1, "wasd = mover   f = golpear   r = reiniciar",
           90);
}

void game_render(void *mem, size_t size, Framebuffer *fb) {
  (void)size;
  Estado *e = estado(mem);

  // Siempre limpiar primero: el framebuffer conserva lo del frame anterior.
  fb_limpiar(fb);

  for (int x = 0; x < SALA_ANCHO; x++) {
    fb_poner(fb, x, 0, '#', 33);
    fb_poner(fb, x, SALA_ALTO - 1, '#', 33);
  }
  for (int y = 0; y < SALA_ALTO; y++) {
    fb_poner(fb, 0, y, '#', 33);
    fb_poner(fb, SALA_ANCHO - 1, y, '#', 33);
  }

  // El destello del golpe va antes que los enemigos y el jugador, para que
  // nunca los tape.
  if (e->golpe_frames > 0) {
    for (int dy = -ALCANCE_GOLPE; dy <= ALCANCE_GOLPE; dy++)
      for (int dx = -ALCANCE_GOLPE; dx <= ALCANCE_GOLPE; dx++)
        if (dx || dy)
          if (!es_pared(e->x + dx, e->y + dy))
            fb_poner(fb, e->x + dx, e->y + dy, '*', 93);
  }

  for (int i = 0; i < MAX_ENEMIGOS; i++)
    if (e->enemigos[i].vivo)
      fb_poner(fb, e->enemigos[i].x, e->enemigos[i].y, 'g', 31);

  fb_poner(fb, e->x, e->y, '@', 36);

  if (e->fase == GANASTE)
    texto_centrado(fb, SALA_ALTO / 2, " GANASTE - r para jugar de nuevo ", 32);
  else if (e->fase == PERDISTE)
    texto_centrado(fb, SALA_ALTO / 2, " MORISTE - r para jugar de nuevo ", 31);

  dibujar_hud(fb, e);
}
