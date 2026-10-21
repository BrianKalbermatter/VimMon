// Ejemplo del renderer 2D: un rectangulo, un degradado y un sprite con
// espejo, para ver que los tres caminos del contrato dibujan.
//
//   make example2d && ./build/hello_2d
//   ./build/hello_2d --captura /tmp/frame.bmp   (un frame y sale)

#include "plugins/renderer2d/renderer2d.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *captura = (argc > 2 && strcmp(argv[1], "--captura") == 0) ? argv[2] : NULL;

    Renderer2D *r = &gpu2d_sdl_renderer2d;
    if (r->init("VimMon 2D", 960, 540, 270) != 0) return 1;

    // Una flecha de 8x8 que apunta a la derecha: si el espejo anda, la
    // segunda apunta a la izquierda.
    uint32_t pix[8 * 8];
    for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
            int dy = y < 4 ? 3 - y : y - 4;
            pix[y * 8 + x] = (x <= 7 - dy * 2) ? RGB2(0xfb, 0xb9, 0x54) : 0;
        }
    Textura2D flecha = r->textura_desde_pixeles(pix, 8, 8);

    while (!r->poll_quit() && !r->tecla(T2_ESCAPE)) {
        float mx = 0, my = 0;
        r->mouse_pos(&mx, &my);

        r->frame_begin(RGB2(0x0d, 0x0b, 0x12));
        r->rect((Rect2D){ 10, 10, 40, 20 }, RGB2(0x78, 0x8a, 0x87));

        V2 esquinas[4] = { { 60, 10 }, { 140, 10 }, { 130, 50 }, { 70, 50 } };
        uint32_t colores[4] = { RGB2(0x99, 0x3d, 0x41), RGB2(0x99, 0x3d, 0x41),
                                ARGB(0, 0, 0, 0), ARGB(0, 0, 0, 0) };
        r->cuad(esquinas, colores);

        r->sprite(flecha, (Rect2D){ 0, 0, 8, 8 }, (Rect2D){ 10, 60, 32, 32 }, BLANCO2, 0);
        r->sprite(flecha, (Rect2D){ 0, 0, 8, 8 }, (Rect2D){ 50, 60, 32, 32 }, BLANCO2, 1);

        // Un cuadradito donde esta el mouse, en pixeles del lienzo.
        r->rect((Rect2D){ (float)(int)mx - 2, (float)(int)my - 2, 4, 4 }, BLANCO2);
        r->frame_end();

        if (captura) {
            int fallo = gpu2d_sdl_capturar_bmp(captura);
            printf("lienzo %dx%d escala %d -> %s\n", r->ancho_interno(), r->alto_interno(),
                   r->escala(), fallo ? "fallo" : captura);
            break;
        }
        r->delay_ms(16);
    }
    r->shutdown();
    return 0;
}
