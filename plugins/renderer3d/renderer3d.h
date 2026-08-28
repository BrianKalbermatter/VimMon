#ifndef VIMMON_RENDERER3D_H
#define VIMMON_RENDERER3D_H

#include <stdint.h>

// ============================================================
// VimMon — CONTRATO DE RENDERER 3D v1.0
//
// Hermano de renderer.h. Misma idea, otra dimension: un struct de
// punteros a funcion que CUALQUIER backend puede rellenar. Este header
// no incluye SDL a proposito, igual que el 2D.
//
// ── LOS LIMITES SON EL DISENO ────────────────────────────────
// Este motor NO es de proposito general. Hace exactamente dos cosas:
//
//   1. GEOMETRIA con profundidad: cubos y planos texturados. El mundo.
//   2. BILLBOARDS: sprites que siempre miran a la camara. Los enemigos,
//      los items, todo lo que "vive" en el mundo.
//
// Es el motor de DOOM y Duke Nukem 3D, no el de un mundo abierto.
// Y esa limitacion no es pobreza: es lo que hace que un solo programador
// pueda entenderlo entero y que corra a 60fps en cualquier cosa.
//
// ── PIXEL ART DE VERDAD ──────────────────────────────────────
// Todo se dibuja a una resolucion INTERNA chica (por ejemplo 480x270) y
// recien al final se escala a la ventana sin suavizado. Eso da dos cosas
// gratis:
//   - los pixeles son cuadrados grandes y parejos, no un desenfoque que
//     cambia de tamano segun cuan grande abriste la ventana
//   - la GPU pinta ~8 veces menos pixeles que a 1080p
// ============================================================

// ── Tipos basicos ───────────────────────────────────────────
typedef struct { float x, y, z; } V3;

// Color ARGB de 32 bits: 0xAARRGGBB, igual que en renderer.h.
#define RGB3(r, g, b) \
    (0xFF000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define BLANCO 0xFFFFFFFFu

// Handles OPACOS. Afuera son numeros y nada mas: nadie puede tocar la
// textura ni el buffer de vertices por su cuenta. Misma regla que el
// framebuffer privado del backend 2D, aplicada a recursos de GPU.
typedef uint32_t Malla;
typedef uint32_t Textura;
#define MALLA_NULA    0u
#define TEXTURA_NULA  0u

// La camara. Se define por donde ESTA y que MIRA, que es como piensa
// un humano, no con una matriz que hay que armar a mano.
typedef struct {
    V3    pos;
    V3    mira_a;
    float fov_grados;   // apertura vertical; 60-75 es lo tipico
    float cerca, lejos; // plano cercano y lejano del frustum
} Camara3D;

// Teclas portables. Mismo enum que el backend 2D para que el juego que
// pasa de 2D a 3D no tenga que reaprender nada.
typedef enum {
    T3_ESCAPE = 0, T3_SPACE,
    T3_UP, T3_DOWN, T3_LEFT, T3_RIGHT,
    T3_W, T3_A, T3_S, T3_D,
    T3_COUNT
} Tecla3D;

// Botones del mouse. Tres y nada mas: es lo que tiene cualquier mouse.
typedef enum {
    M3_IZQ = 0, M3_DER, M3_MEDIO,
    M3_COUNT
} Boton3D;

// ── El contrato ─────────────────────────────────────────────
typedef struct {
    // Ciclo de vida -------------------------------------------------
    // ancho/alto_interno es el lienzo real donde se dibuja (chico).
    // ancho/alto_ventana es a cuanto se estira despues (grande).
    int  (*init)(const char *titulo,
                 int ancho_ventana, int alto_ventana,
                 int ancho_interno, int alto_interno);
    void (*shutdown)(void);

    // Recursos ------------------------------------------------------
    Malla   (*malla_cubo)(void);
    Malla   (*malla_plano)(void);
    Textura (*textura_solida)(uint32_t argb);   // 1x1 pixel de un color
    Textura (*textura_desde_pixeles)(const uint32_t *argb, int w, int h);

    // Un frame ------------------------------------------------------
    void (*frame_begin)(const Camara3D *cam, uint32_t color_cielo);
    void (*dibujar_malla)(Malla m, V3 pos, V3 rot_grados, V3 escala,
                          Textura t, uint32_t tinte);
    // El billboard NO recibe rotacion: siempre encara a la camara.
    // Esa es toda su gracia.
    void (*dibujar_billboard)(Textura t, V3 pos, float ancho, float alto,
                              uint32_t tinte);
    void (*frame_end)(void);   // escala el lienzo interno y presenta

    // Plataforma ----------------------------------------------------
    int      (*poll_quit)(void);
    int      (*tecla)(Tecla3D k);

    // ── Mouse ──
    // El movimiento se pide RELATIVO: cuanto se movio DESDE LA ULTIMA VEZ que
    // preguntaste, no en que pixel esta. Para mirar alrededor es lo unico que
    // sirve: la camara gira segun cuanto corriste el mouse, y el puntero no
    // tiene que existir ni chocar contra el borde de la pantalla.
    //
    // Leer CONSUME el movimiento acumulado: dos llamadas seguidas en el mismo
    // frame dan el desplazamiento la primera y cero la segunda. Se pide UNA vez
    // por frame.
    void     (*mouse_delta)(float *dx, float *dy);
    int      (*mouse_boton)(Boton3D b);

    // Captura el mouse dentro de la ventana y esconde el puntero. Es lo que
    // hace todo shooter al empezar la partida.
    void     (*mouse_capturar)(int activar);
    uint32_t (*ticks_ms)(void);
    void     (*delay_ms)(uint32_t ms);

    // Consultas -----------------------------------------------------
    int (*ancho_interno)(void);
    int (*alto_interno)(void);
} Renderer3D;

// Backend concreto sobre SDL_GPU (definido en gpu_sdl.c).
extern Renderer3D gpu_sdl_renderer3d;

// VERIFICAR SIN PANTALLA. Baja el lienzo interno de la GPU a RAM y lo
// escribe como BMP. Llamar DESPUES de frame_end(), que es cuando el lienzo
// tiene el frame dibujado. Devuelve 0 si salio bien.
//
// No es parte del contrato (no esta en el vtable) a proposito: es una
// herramienta de diagnostico del backend, no algo que un juego use. Existe
// porque con GPU un binding mal puesto no da error: da pantalla negra. La
// unica forma de saber que dibujo es MIRAR los pixeles.
int gpu_sdl_capturar_bmp(const char *ruta);

#endif // VIMMON_RENDERER3D_H
