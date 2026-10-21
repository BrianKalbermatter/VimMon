#ifndef VIMMON_RENDERER2D_H
#define VIMMON_RENDERER2D_H

#include <stdint.h>

// ============================================================
// VimMon — CONTRATO DE RENDERER 2D
//
// Hermano de renderer3d.h. Un struct de punteros a funcion que CUALQUIER
// backend puede rellenar. Este header no incluye SDL a proposito: hoy lo
// rellena SDL_GPU (gpu2d_sdl.c), y manana lo puede rellenar un backend
// WebGL/WebGPU para exportar el juego al navegador sin tocar el juego.
//
// ── QUE DIBUJA ───────────────────────────────────────────────
// Una sola cosa: CUADRILATEROS. Un sprite es un cuadrilatero con un
// pedazo de textura; un rectangulo de color es un cuadrilatero con una
// textura blanca de 1x1; la cara inclinada de un muro es un cuadrilatero
// con las cuatro esquinas donde uno quiera. Todo lo demas se arma con eso.
//
// ── EN QUE ORDEN ─────────────────────────────────────────────
// No hay z-buffer: lo que se dibuja despues tapa a lo de antes (el
// algoritmo del pintor). El juego ordena. Es lo mismo que hace PixiJS con
// zIndex, y para 2D visto desde arriba es lo correcto: la profundidad la
// decide la Y de los pies, no una distancia a la camara.
//
// ── PIXEL ART DE VERDAD ──────────────────────────────────────
// Se dibuja a un lienzo INTERNO chico y se escala a la ventana por un
// factor ENTERO (2x, 3x...). Cada pixel del arte es un cuadrado parejo. El
// tamano interno sale del alto de la ventana y del alto que se pide en
// init, y cambia si la ventana cambia: hay que preguntarlo cada frame.
// ============================================================

typedef struct { float x, y; } V2;

// Rectangulo en pixeles: de un lienzo, de una textura o del mundo.
typedef struct { float x, y, w, h; } Rect2D;

// Color ARGB de 32 bits: 0xAARRGGBB, igual que en renderer.h y renderer3d.h.
#define ARGB(a, r, g, b) \
    (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define RGB2(r, g, b) ARGB(0xFF, r, g, b)
#define BLANCO2       0xFFFFFFFFu

// Handle OPACO: afuera es un numero y nada mas.
typedef uint32_t Textura2D;
#define TEXTURA2D_NULA 0u

// Teclas portables. Mas que en el 3D: un juego 2D usa medio teclado
// (habilidades en Q W E R, correr con Shift, atajos del editor...).
typedef enum {
    T2_ESCAPE = 0, T2_SPACE, T2_SHIFT, T2_CTRL, T2_TAB, T2_ENTER,
    T2_UP, T2_DOWN, T2_LEFT, T2_RIGHT,
    T2_A, T2_B, T2_C, T2_D, T2_E, T2_F, T2_G, T2_H, T2_I, T2_J, T2_K, T2_L, T2_M,
    T2_N, T2_O, T2_P, T2_Q, T2_R, T2_S, T2_T, T2_U, T2_V, T2_W, T2_X, T2_Y, T2_Z,
    T2_0, T2_1, T2_2, T2_3, T2_4, T2_5, T2_6, T2_7, T2_8, T2_9,
    T2_F1, T2_F2, T2_F3, T2_F4,
    T2_COUNT
} Tecla2D;

typedef enum { M2_IZQ = 0, M2_DER, M2_MEDIO, M2_COUNT } Boton2D;

// ── El contrato ─────────────────────────────────────────────
typedef struct {
    // Ciclo de vida -------------------------------------------------
    // alto_objetivo: cuantos pixeles de arte entran de alto (el navegador
    // usa 480). La escala es la entera mas cercana a alto_ventana/objetivo.
    int  (*init)(const char *titulo, int ancho_ventana, int alto_ventana,
                 int alto_objetivo);
    void (*shutdown)(void);

    // Recursos ------------------------------------------------------
    Textura2D (*textura_desde_pixeles)(const uint32_t *argb, int w, int h);
    Textura2D (*textura_cargar)(const char *ruta_png);   // NULA si falla
    void      (*textura_tamano)(Textura2D t, int *w, int *h);

    // Un frame ------------------------------------------------------
    // Entre frame_begin y frame_end solo se ANOTA: los cuadrilateros se
    // juntan en RAM y en frame_end viajan todos juntos a la GPU.
    void (*frame_begin)(uint32_t color_fondo);

    // Lo que se dibuja despues se corre (-x, -y). (0,0) = coordenadas del
    // lienzo, para el HUD. El juego redondea: el pixel art no tolera
    // medio pixel.
    void (*camara)(float x, float y);

    // Un pedazo `origen` (pixeles de la textura) de `t` en `destino`.
    // espejo != 0 lo da vuelta en horizontal.
    void (*sprite)(Textura2D t, Rect2D origen, Rect2D destino,
                   uint32_t tinte, int espejo);
    void (*rect)(Rect2D r, uint32_t argb);
    // Cuatro esquinas en orden (ida y vuelta, no cruzadas), un color cada
    // una: se interpolan, asi sale un degradado gratis.
    void (*cuad)(const V2 p[4], const uint32_t argb[4]);

    void (*frame_end)(void);   // sube, dibuja, escala y presenta

    // Plataforma ----------------------------------------------------
    int      (*poll_quit)(void);
    int      (*tecla)(Tecla2D k);
    // Posicion del mouse en pixeles del LIENZO (no de la ventana): lo que
    // el juego necesita para saber sobre que pixel de arte esta.
    void     (*mouse_pos)(float *x, float *y);
    int      (*mouse_boton)(Boton2D b);
    int      (*mouse_dentro)(void);   // 1 si el puntero esta sobre la ventana
    uint32_t (*ticks_ms)(void);
    void     (*delay_ms)(uint32_t ms);

    // Consultas -----------------------------------------------------
    int (*ancho_interno)(void);
    int (*alto_interno)(void);
    int (*escala)(void);
} Renderer2D;

// Backend concreto sobre SDL_GPU (definido en gpu2d_sdl.c).
extern Renderer2D gpu2d_sdl_renderer2d;

// VERIFICAR SIN PANTALLA: baja el ultimo frame del lienzo a un BMP.
// Llamar despues de frame_end(). Devuelve 0 si salio bien. Fuera del
// vtable a proposito, como en el 3D: es diagnostico, no algo del juego.
int gpu2d_sdl_capturar_bmp(const char *ruta);

#endif // VIMMON_RENDERER2D_H
