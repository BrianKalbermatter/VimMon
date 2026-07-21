#ifndef VIMMON_RENDERER_H
#define VIMMON_RENDERER_H

#include <stdint.h>

// ============================================================
// VimMon — RENDERER CONTRACT v1.0
// Interfaz ABSTRACTA de dibujo. NO incluye SDL a propósito.
//
// Este header es un "vtable": un struct de punteros a función.
// Cualquier backend (framebuffer SDL2 hoy, Vulkan o bare-metal
// mañana) rellena estos punteros. El resto del sistema dibuja a
// través de esta interfaz y NUNCA sabe qué backend hay debajo.
// ============================================================

// Color en formato ARGB de 32 bits: 0xAARRGGBB.
// Armamos un color desde componentes 0..255. El alfa queda en 0xFF (opaco).
#define RGB(r, g, b) \
    (0xFF000000u | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

// Teclas portables. El backend las traduce a sus códigos internos
// (scancodes de SDL, por ejemplo). Así el juego no depende de SDL.
typedef enum {
    KEY_ESCAPE = 0,
    KEY_SPACE,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_COUNT
} Key;

// El contrato. Cada backend expone un valor de este tipo.
typedef struct {
    // Ciclo de vida --------------------------------------------------
    int  (*init)(const char *title, int width, int height); // 0 si OK
    void (*shutdown)(void);

    // Dimensiones del lienzo ----------------------------------------
    int  (*width)(void);
    int  (*height)(void);

    // Primitivas de dibujo (escriben en el framebuffer interno) ------
    void (*clear)(uint32_t color);
    void (*put_pixel)(int x, int y, uint32_t color);
    void (*fill_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_line)(int x0, int y0, int x1, int y1, uint32_t color);

    // Presentar: sube el framebuffer a la pantalla ------------------
    void (*present)(void);

    // Entrada y tiempo (los provee la plataforma) -------------------
    int      (*poll_quit)(void);        // procesa eventos; 1 si pidieron cerrar
    int      (*key_down)(Key k);        // 1 si la tecla está presionada ahora
    uint32_t (*ticks_ms)(void);         // milisegundos desde el arranque
    void     (*delay_ms)(uint32_t ms);  // dormir (para limitar los FPS)
} Renderer;

// Backend concreto basado en framebuffer SDL2 (definido en sdl_fb.c).
extern Renderer sdl_fb_renderer;

#endif // VIMMON_RENDERER_H
