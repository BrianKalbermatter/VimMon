// host.c — plataforma. Esto NO se recarga nunca.
// Responsabilidades: ser dueño de la memoria del juego, poner la terminal en
// modo raw, leer teclas sin bloquear, mantener el ritmo de frames, volcar el
// framebuffer, cargar game.so y recargarlo en caliente cuando cambia en disco.
//
// Lo que el host NO sabe: como es tu estado. Te presta un bloque de bytes.

#include "platform.h"
#include <dlfcn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define LIB_PATH "./game.so"
#define LIB_LIVE "./game_live.so"
#define RELOAD_RETRIES 5

// 16 MB de una sola vez. En un juego asi no vas a necesitar malloc nunca mas:
// todo tu estado sale de este bloque, y por eso sobrevive las recargas.
#define GAME_MEM_SIZE (16u * 1024u * 1024u)

#define FPS 120
#define FRAME_NS (1000000000L / FPS)

// Cada cuantos frames revisamos si game.so cambio. Ya no hay una espera
// bloqueante donde meter el chequeo, asi que lo hacemos periodico.
// Se calcula sobre FPS para que siga siendo medio segundo si cambias el ritmo.
#define RELOAD_CHECK_FRAMES (FPS / 2)

typedef struct {
  void *handle;
  void (*init)(void *, size_t);
  int (*update)(void *, size_t, Input, float);
  void (*render)(void *, size_t, Framebuffer *);
  time_t stamp; // mtime de game.so cuando lo cargamos
} GameCode;

// --- La terminal -------------------------------------------------------------
// Si el proceso muere en modo raw sin restaurar, la shell queda inservible:
// no ves lo que tipeas y Ctrl-C no responde. Por eso atexit + seniales.

static struct termios term_original;
static int term_modificada = 0;

static void term_restaurar(void) {
  if (!term_modificada)
    return;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_original);
  fputs("\033[?25h" RESET "\n", stdout); // cursor visible de nuevo
  fflush(stdout);
  term_modificada = 0;
}

static void on_signal(int sig) {
  term_restaurar();
  // _exit y no exit: dentro de un handler no es seguro llamar a exit().
  _exit(128 + sig);
}

static int term_raw(void) {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "[host] stdin no es una terminal\n");
    return 0;
  }
  if (tcgetattr(STDIN_FILENO, &term_original) != 0) {
    perror("[host] tcgetattr");
    return 0;
  }
  atexit(term_restaurar);
  signal(SIGINT, on_signal);
  signal(SIGTERM, on_signal);

  struct termios raw = term_original;
  // ICANON fuera: el kernel deja de guardar las teclas hasta el Enter.
  // ECHO fuera: las teclas no se imprimen solas encima del dibujo.
  raw.c_lflag &= ~(unsigned)(ECHO | ICANON);
  raw.c_cc[VMIN] = 0;  // read() vuelve ya mismo, aunque no haya nada
  raw.c_cc[VTIME] = 0; // sin timeout
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
    perror("[host] tcsetattr");
    return 0;
  }
  term_modificada = 1;
  fputs("\033[?25l\033[2J", stdout); // cursor invisible, pantalla limpia
  return 1;
}

static void aviso_tamanio(void) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0)
    return;
  if (ws.ws_col < FB_ANCHO || ws.ws_row < FB_ALTO + 1)
    fprintf(stderr,
            "[host] la terminal es de %dx%d y el juego necesita %dx%d. "
            "Se va a ver cortado.\n",
            ws.ws_col, ws.ws_row, FB_ANCHO, FB_ALTO + 1);
}

static long ahora_ns(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000L + ts.tv_nsec;
}

// --- Volcado del framebuffer -------------------------------------------------
// Un solo write por frame. Muchos printf sueltos parpadean y son lentos.
// El codigo de color se emite solo cuando cambia respecto a la celda anterior.

static void fb_volcar(const Framebuffer *fb, char *salida, size_t cap) {
  size_t n = 0;
  const char *home = "\033[H"; // cursor arriba a la izquierda, sin borrar
  memcpy(salida + n, home, 3);
  n += 3;

  int actual = -1; // -1 = todavia no emitimos ningun color
  for (int y = 0; y < fb->alto; y++) {
    for (int x = 0; x < fb->ancho; x++) {
      Cell c = fb->cells[y * fb->ancho + x];
      if ((int)c.color != actual) {
        n += (size_t)snprintf(salida + n, cap - n,
                              c.color ? "\033[%um" : RESET, c.color);
        actual = c.color;
      }
      salida[n++] = c.ch;
    }
    salida[n++] = '\n';
  }
  n += (size_t)snprintf(salida + n, cap - n, RESET);
  fwrite(salida, 1, n, stdout);
  fflush(stdout);
}

// --- Hot reload --------------------------------------------------------------

static time_t lib_mtime(void) {
  struct stat st;
  if (stat(LIB_PATH, &st) != 0)
    return 0;
  return st.st_mtime;
}

// Copiamos game.so a game_live.so y cargamos la copia.
// Asi el `make` de la otra terminal puede reescribir game.so libremente
// sin tocar el archivo que el proceso tiene mapeado en memoria.
static int copy_lib(void) {
  FILE *src = fopen(LIB_PATH, "rb");
  if (!src)
    return 0;
  FILE *dst = fopen(LIB_LIVE, "wb");
  if (!dst) {
    fclose(src);
    return 0;
  }
  char buf[8192];
  size_t n;
  while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
    fwrite(buf, 1, n, dst);
  fclose(src);
  fclose(dst);
  return 1;
}

static void unload_game(GameCode *code) {
  if (code->handle)
    dlclose(code->handle);
  code->handle = NULL;
  code->init = NULL;
  code->update = NULL;
  code->render = NULL;
}

static int load_game(GameCode *code) {
  unload_game(code);

  if (!copy_lib()) {
    fprintf(stderr, "[host] no pude copiar %s\n", LIB_PATH);
    return 0;
  }

  code->handle = dlopen(LIB_LIVE, RTLD_NOW);
  if (!code->handle) {
    fprintf(stderr, "[host] dlopen fallo: %s\n", dlerror());
    return 0;
  }

  // dlsym puede devolver NULL legitimamente, por eso se limpia dlerror antes.
  dlerror();
  *(void **)(&code->init) = dlsym(code->handle, "game_init");
  *(void **)(&code->update) = dlsym(code->handle, "game_update");
  *(void **)(&code->render) = dlsym(code->handle, "game_render");
  char *err = dlerror();
  if (err) {
    fprintf(stderr, "[host] dlsym fallo: %s\n", err);
    unload_game(code);
    return 0;
  }

  code->stamp = lib_mtime();
  return 1;
}

// Un .so recien escrito puede estar a medio grabar; reintentamos un toque.
static int load_game_retry(GameCode *code) {
  struct timespec pausa = {0, 100 * 1000 * 1000}; // 100 ms
  for (int i = 0; i < RELOAD_RETRIES; i++) {
    if (load_game(code))
      return 1;
    nanosleep(&pausa, NULL);
  }
  return 0;
}

// --- main --------------------------------------------------------------------

int main(void) {
  srand((unsigned)time(NULL));

  GameCode code = {0};
  if (!load_game_retry(&code)) {
    fprintf(stderr, "[host] no pude cargar el juego. Corre `make` primero.\n");
    return 1;
  }

  // El estado es del host: sobrevive a todas las recargas. El host no sabe
  // que hay adentro, y justamente por eso podes cambiarle la forma sin
  // recompilarlo.
  void *mem = calloc(1, GAME_MEM_SIZE);
  Cell *cells = calloc((size_t)FB_ANCHO * FB_ALTO, sizeof(Cell));
  size_t cap = (size_t)FB_ALTO * (FB_ANCHO * 12 + 8) + 64;
  char *salida = malloc(cap);
  if (!mem || !cells || !salida) {
    fprintf(stderr, "[host] sin memoria\n");
    free(mem);
    free(cells);
    free(salida);
    return 1;
  }

  Framebuffer fb = {FB_ANCHO, FB_ALTO, cells};

  aviso_tamanio();
  if (!term_raw()) {
    free(mem);
    free(cells);
    free(salida);
    return 1;
  }

  code.init(mem, GAME_MEM_SIZE);

  int playing = 1;
  long frames = 0;
  float dt = 1.0f / FPS;
  const char *estado = "";

  while (playing) {
    long inicio = ahora_ns();

    // Input: se lee todo lo que haya llegado y se lo pasa al juego.
    // Si no hay nada, n queda en 0 y el frame sigue igual.
    char teclas[32];
    ssize_t leidos = read(STDIN_FILENO, teclas, sizeof teclas);
    Input in = {teclas, leidos > 0 ? (int)leidos : 0};

    playing = code.update(mem, GAME_MEM_SIZE, in, dt);

    code.render(mem, GAME_MEM_SIZE, &fb);

    // HUD del host, encima de lo que dibujo el juego: FPS y estado de recarga.
    char hud[FB_ANCHO + 1];
    snprintf(hud, sizeof hud, " %2d fps  q=salir  %s", (int)(1.0f / dt),
             estado);
    fb_texto(&fb, 0, FB_ALTO - 1, hud, 90);

    fb_volcar(&fb, salida, cap);

    // Recarga en caliente: cada medio segundo miramos si game.so cambio.
    if (++frames % RELOAD_CHECK_FRAMES == 0 && lib_mtime() != code.stamp) {
      estado = load_game_retry(&code) ? GREEN "recargado" RESET
                                      : RED "recarga fallida" RESET;
    }

    // Dormimos lo que sobra del frame. Sin esto, el bucle gira al maximo y te
    // funde un nucleo al 100% para dibujar lo mismo.
    long usado = ahora_ns() - inicio;
    long resto = FRAME_NS - usado;
    if (resto > 0) {
      struct timespec s = {0, resto};
      nanosleep(&s, NULL);
    }
    // dt real del frame que acaba de pasar, para el proximo.
    dt = (float)(ahora_ns() - inicio) / 1e9f;
  }

  term_restaurar();
  free(mem);
  free(cells);
  free(salida);
  unload_game(&code);
  printf(CYAN "Chau!" RESET "\n");
  return 0;
}
