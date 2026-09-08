#ifndef PLATFORM_H
#define PLATFORM_H

// platform.h — el contrato entre el host y game.so.
//
// Esto es territorio de la plataforma: la terminal, el framebuffer, el input,
// el reloj. NO va nada de gameplay aca. Tus structs (jugador, enemigos,
// habitaciones, habilidades) van en TU header, que solo incluye game.so.
//
// El host no conoce tu estado. Te presta un bloque de memoria y vos lo
// interpretas como quieras: podes rediseniar todo tu GameState sin recompilar
// ni reiniciar el host.

#include <stddef.h>

// <--- ANSI Colores --->
// Para escribir texto suelto. Dentro del framebuffer se usa Cell.color.
#define RESET "\033[0m"
#define BOLD "\033[1m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"

// --- Tamanio del campo de juego ---
// Estos son TECHOS, no el tamanio real. El host mide la terminal al arrancar y
// arma el framebuffer del tamanio que entre, sin pasarse de estos numeros.
//
// Por que un techo y no la terminal a secas: para acotar la memoria del
// framebuffer y del buffer de salida, que se reservan una sola vez.
//
// El alto real SIEMPRE es menor que las filas de la terminal. Cada frame se
// escribe una linea terminada en \n por fila: si fueran tantas como la
// terminal, esta scrollea y el dibujo se va para arriba, fuera de la vista. Se
// ve la pantalla en negro y parece que no dibuja, pero dibuja perfecto.
#define FB_ANCHO 200
#define FB_ALTO 80

// Abajo de esto el juego no entra y el host se niega a arrancar.
#define FB_MIN_ANCHO 80
#define FB_MIN_ALTO 20

// --- Framebuffer ---
// Una celda = un caracter + su color. Un char solo no alcanza porque no
// guarda color, y vos ya venias usando ANSI.
// Naranja de verdad. No existe en los 8 colores basicos de ANSI: el mas cercano
// es el amarillo (33), que segun el tema de la terminal se ve mostaza. El 208
// es de la paleta de 256, que el host emite con otra secuencia (ver Cell.color).
#define NARANJA 208

typedef struct {
  char ch;
  // 0        = color por defecto
  // 1..107   = codigo ANSI clasico (31..37 normales, 90..97 brillantes)
  // 108..255 = indice de la paleta de 256 colores, como NARANJA
  unsigned char color;
} Cell;

typedef struct {
  int ancho, alto;
  Cell *cells; // ancho*alto celdas, fila por fila
} Framebuffer;

// --- Input ---
// Bytes crudos leidos del teclado en este frame. Puede venir vacio (n == 0):
// eso es normal, significa que el jugador no toco nada.
//
// Ojo: las flechas NO son un byte. Llegan como la secuencia ESC '[' 'A'
// (arriba), 'B' (abajo), 'C' (derecha), 'D' (izquierda). Si vas a usarlas,
// tenes que detectar los tres bytes juntos. WASD es un byte y es mas simple.
//
// El mouse tambien lo resuelve el host. La terminal reporta los clicks como
// una secuencia de escape (ESC [ < boton ; columna ; fila M), pero eso es
// ruido de plataforma: el host la parsea y te deja el resultado ya masticado
// en coordenadas del framebuffer.
//
// Los flags valen 1 SOLO en el frame en que ocurre el evento, no mientras se
// mantiene. arrastrando es la excepcion: vale 1 en cada frame que el cursor se
// mueve con el boton izquierdo apretado.
typedef struct {
  int x, y; // ultima posicion conocida, en celdas del framebuffer

  int izq_apretado; // se apreto el boton izquierdo
  int izq_soltado;  // se solto el boton izquierdo
  int arrastrando;  // se movio el cursor con el izquierdo apretado

  int der_apretado;    // se apreto el boton derecho
  int der_soltado;     // se solto el boton derecho
  int der_arrastrando; // se movio el cursor con el derecho apretado
} Mouse;

typedef struct {
  const char *bytes;
  int n;
  Mouse mouse;
} Input;

// --- API que game.so tiene que exportar ---
// El host resuelve estos tres simbolos con dlsym en cada recarga. Si le
// cambias el nombre o la firma a alguno, tocalo tambien en host.c.
//
// mem: bloque de GAME_MEM_SIZE bytes, en cero la primera vez, y que sobrevive
//      todas las recargas en caliente. Ahi vive tu estado.

// Corre una sola vez, antes del primer frame. ancho y alto son los del
// framebuffer que el host acaba de armar: el juego arma su terreno con esos
// numeros, no con FB_ANCHO/FB_ALTO, que son solo el techo.
void game_init(void *mem, size_t size, int ancho, int alto);

// Corre una vez por frame. dt son los segundos que paso desde el frame
// anterior: usalo para que la velocidad no dependa de los FPS.
// Devolve 0 para que el host corte el bucle y salga.
int game_update(void *mem, size_t size, Input in, float dt);

// Corre despues de update. Escribi en fb; el host se encarga de volcarlo a la
// terminal de una sola pasada.
void game_render(void *mem, size_t size, Framebuffer *fb);

// --- Utilidades de dibujo ---

static inline void fb_limpiar(Framebuffer *fb) {
  for (int i = 0; i < fb->ancho * fb->alto; i++) {
    fb->cells[i].ch = ' ';
    fb->cells[i].color = 0;
  }
}

// Recorta fuera de pantalla. Sin este chequeo, un enemigo con x negativo te
// escribe fuera del array y te corrompe memoria sin avisar.
static inline void fb_poner(Framebuffer *fb, int x, int y, char ch,
                            unsigned char color) {
  if (x < 0 || x >= fb->ancho || y < 0 || y >= fb->alto)
    return;
  fb->cells[y * fb->ancho + x].ch = ch;
  fb->cells[y * fb->ancho + x].color = color;
}

static inline void fb_texto(Framebuffer *fb, int x, int y, const char *s,
                            unsigned char color) {
  for (int i = 0; s[i]; i++)
    fb_poner(fb, x + i, y, s[i], color);
}

#endif // PLATFORM_H
