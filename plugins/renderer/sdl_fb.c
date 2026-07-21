// ============================================================
// VimMon — BACKEND FRAMEBUFFER (SDL2)
//
// Implementa el contrato de renderer.h dibujando "a mano" sobre
// un bloque de memoria (el framebuffer) y copiándolo a la ventana.
//
// REGLA DE ORO: el framebuffer 'pixels' es PRIVADO (static). Nadie
// afuera ve el puntero. Solo se toca mediante las primitivas de acá.
// Así el formato, el ancho y los límites viven en UN solo lugar.
// ============================================================

#include "renderer.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

// --- Estado interno del backend (todo static = invisible afuera) ---
static SDL_Window   *g_win     = NULL;
static SDL_Renderer *g_ren     = NULL;
static SDL_Texture  *g_tex     = NULL;
static uint32_t     *g_pixels  = NULL;   // el framebuffer: ancho*alto enteros
static int           g_width   = 0;
static int           g_height  = 0;

// Traducción de nuestras teclas portables a scancodes de SDL.
// Se indexa con el enum Key, así que el orden debe coincidir.
static const SDL_Scancode g_scancodes[KEY_COUNT] = {
    [KEY_ESCAPE] = SDL_SCANCODE_ESCAPE,
    [KEY_SPACE]  = SDL_SCANCODE_SPACE,
    [KEY_UP]     = SDL_SCANCODE_UP,
    [KEY_DOWN]   = SDL_SCANCODE_DOWN,
    [KEY_LEFT]   = SDL_SCANCODE_LEFT,
    [KEY_RIGHT]  = SDL_SCANCODE_RIGHT,
    [KEY_W]      = SDL_SCANCODE_W,
    [KEY_A]      = SDL_SCANCODE_A,
    [KEY_S]      = SDL_SCANCODE_S,
    [KEY_D]      = SDL_SCANCODE_D,
};

// ------------------------------------------------------------
// Ciclo de vida
// ------------------------------------------------------------
static int fb_init(const char *title, int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[sdl_fb] SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    g_win = SDL_CreateWindow(title,
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             width, height, 0);
    if (g_win == NULL) {
        fprintf(stderr, "[sdl_fb] CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
    if (g_ren == NULL) {
        fprintf(stderr, "[sdl_fb] CreateRenderer: %s\n", SDL_GetError());
        return 1;
    }

    // Textura "streaming": pensada para reescribirse cada frame.
    // ARGB8888 = 4 bytes por píxel, mismo layout que nuestro uint32_t.
    g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, width, height);
    if (g_tex == NULL) {
        fprintf(stderr, "[sdl_fb] CreateTexture: %s\n", SDL_GetError());
        return 1;
    }

    // El framebuffer: un array plano de ancho*alto enteros.
    g_pixels = malloc((size_t)width * (size_t)height * sizeof(uint32_t));
    if (g_pixels == NULL) {
        fprintf(stderr, "[sdl_fb] sin memoria para el framebuffer\n");
        return 1;
    }

    g_width  = width;
    g_height = height;
    return 0;
}

static void fb_shutdown(void)
{
    free(g_pixels);       g_pixels = NULL;
    if (g_tex) SDL_DestroyTexture(g_tex);
    if (g_ren) SDL_DestroyRenderer(g_ren);
    if (g_win) SDL_DestroyWindow(g_win);
    SDL_Quit();
}

static int fb_width(void)  { return g_width;  }
static int fb_height(void) { return g_height; }

// ------------------------------------------------------------
// Primitivas de dibujo
// ------------------------------------------------------------

// put_pixel: el ladrillo de todo. Un píxel en (x, y) vive en el
// índice y*ancho + x del array plano.
static void fb_put_pixel(int x, int y, uint32_t color)
{
    // Guard clause: descartamos lo que cae fuera del lienzo ANTES de
    // tocar la memoria. Si el índice estuviera podrido, nunca lo usamos.
    if (x < 0 || x >= g_width || y < 0 || y >= g_height)
        return;
    g_pixels[y * g_width + x] = color;
}

static void fb_clear(uint32_t color)
{
    // Cantidad de vueltas conocida (ancho*alto) -> for.
    int total = g_width * g_height;
    for (int i = 0; i < total; i++)
        g_pixels[i] = color;
}

static void fb_fill_rect(int x, int y, int w, int h, uint32_t color)
{
    // Recortamos el rectángulo al lienzo para no iterar de más.
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > g_width  ? g_width  : x + w;
    int y1 = y + h > g_height ? g_height : y + h;

    // Recorrer una región 2D con memoria 1D = dos for anidados:
    // el de afuera avanza filas, el de adentro columnas.
    for (int py = y0; py < y1; py++)
        for (int px = x0; px < x1; px++)
            g_pixels[py * g_width + px] = color;
}

// draw_line: algoritmo de Bresenham. Dibuja una recta con solo
// enteros: ni floats ni divisiones. El if de adentro ES el algoritmo,
// decide cuándo avanzar en el eje secundario acumulando el error.
static void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
    int dx =  abs(x1 - x0);
    int dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;   // dirección en x
    int sy = y0 < y1 ? 1 : -1;   // dirección en y
    int err = dx + dy;

    for (;;) {
        fb_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }  // ¿damos un paso en x?
        if (e2 <= dx) { err += dx; y0 += sy; }  // ¿damos un paso en y?
    }
}

// ------------------------------------------------------------
// Presentar
// ------------------------------------------------------------
static void fb_present(void)
{
    // pitch = bytes por fila. Acá es ancho*4 (4 bytes por píxel).
    // Ojo: en otros contextos el pitch puede tener padding y NO ser
    // igual a ancho*4 (te va a pasar con el kernel en FASE 6).
    SDL_UpdateTexture(g_tex, NULL, g_pixels, g_width * (int)sizeof(uint32_t));
    SDL_RenderClear(g_ren);
    SDL_RenderCopy(g_ren, g_tex, NULL, NULL);
    SDL_RenderPresent(g_ren);
}

// ------------------------------------------------------------
// Entrada y tiempo
// ------------------------------------------------------------
static int fb_poll_quit(void)
{
    SDL_Event e;
    int quit = 0;
    // Vaciamos la cola de eventos. SDL_PollEvent además "bombea" el
    // estado del teclado que usa key_down más abajo.
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT)
            quit = 1;
    }
    return quit;
}

static int fb_key_down(Key k)
{
    if (k < 0 || k >= KEY_COUNT)
        return 0;
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    return state[g_scancodes[k]] ? 1 : 0;
}

static uint32_t fb_ticks_ms(void)      { return SDL_GetTicks(); }
static void     fb_delay_ms(uint32_t m){ SDL_Delay(m); }

// ------------------------------------------------------------
// El vtable: acá se "enchufan" todas las funciones de arriba.
// Esto es lo ÚNICO público de este archivo.
// ------------------------------------------------------------
Renderer sdl_fb_renderer = {
    .init      = fb_init,
    .shutdown  = fb_shutdown,
    .width     = fb_width,
    .height    = fb_height,
    .clear     = fb_clear,
    .put_pixel = fb_put_pixel,
    .fill_rect = fb_fill_rect,
    .draw_line = fb_draw_line,
    .present   = fb_present,
    .poll_quit = fb_poll_quit,
    .key_down  = fb_key_down,
    .ticks_ms  = fb_ticks_ms,
    .delay_ms  = fb_delay_ms,
};
