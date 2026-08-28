// ============================================================
// Ejemplo del motor 3D: un pasillo con enemigos billboard.
//
// Todo lo que se dibuja se genera por codigo, sin un solo asset en disco.
// La idea es que puedas correrlo y ver el estilo funcionando antes de
// dibujar el primer sprite.
//
//   WASD / flechas  moverse
//   ESC             salir
// ============================================================

#include "../plugins/renderer3d/renderer3d.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ANCHO_INT   480    // el lienzo REAL donde se dibuja
#define ALTO_INT    270
#define ANCHO_WIN   960    // a cuanto se estira (x2 exacto: pixeles parejos)
#define ALTO_WIN    540

#define S 16               // los sprites son de 16x16, como corresponde

// Un damero, para que se note el escalado del piso al alejarse.
static void tex_damero(uint32_t *p, uint32_t a, uint32_t b)
{
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++)
            p[y*S + x] = ((x/4 + y/4) % 2) ? a : b;
}

// Ladrillos con junta oscura.
static void tex_ladrillo(uint32_t *p)
{
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            int fila  = y / 4;
            int corr  = (fila % 2) ? 2 : 0;      // hiladas trabadas
            int junta = (y % 4 == 0) || ((x + corr) % 8 == 0);
            p[y*S + x] = junta ? RGB3(40, 30, 28) : RGB3(150, 70, 55);
        }
}

// Un bichito. El alfa 0 es lo que el shader descarta: ahi se ve el fondo.
static void tex_enemigo(uint32_t *p)
{
    static const char *arte[S] = {
        "................",
        "................",
        "....XXXXXXXX....",
        "...XXXXXXXXXX...",
        "..XX.XXXXXX.XX..",
        "..XX.XXXXXX.XX..",
        "..XXXXXXXXXXXX..",
        "..XXOOXXXXOOXX..",
        "..XXOOXXXXOOXX..",
        "..XXXXXXXXXXXX..",
        "..XXX.XXXX.XXX..",
        "...XXXXXXXXXX...",
        "....XX.XX.XX....",
        "...XX...X...XX..",
        "..XX....X....XX.",
        "................",
    };
    for (int y = 0; y < S; y++)
        for (int x = 0; x < S; x++) {
            char c = arte[y][x];
            p[y*S + x] = c == 'X' ? RGB3(190, 40, 60)
                       : c == 'O' ? RGB3(250, 230, 90)
                       : 0x00000000u;             // alfa 0 -> discard
        }
}

int main(int argc, char **argv)
{
    // --captura <ruta>: dibuja UN frame, lo guarda y sale. Sirve para
    // verificar que el motor dibuja de verdad sin depender de que haya
    // una pantalla ni de que alguien la mire.
    const char *captura = NULL;
    if (argc >= 3 && strcmp(argv[1], "--captura") == 0) captura = argv[2];

    Renderer3D *r = &gpu_sdl_renderer3d;

    if (r->init("VimMon - motor 3D", ANCHO_WIN, ALTO_WIN, ANCHO_INT, ALTO_INT) != 0) {
        fprintf(stderr, "no se pudo iniciar el motor 3D\n");
        return 1;
    }

    uint32_t buf[S*S];
    tex_damero(buf, RGB3(60,60,70), RGB3(45,45,55));
    Textura t_piso = r->textura_desde_pixeles(buf, S, S);
    tex_ladrillo(buf);
    Textura t_muro = r->textura_desde_pixeles(buf, S, S);
    tex_enemigo(buf);
    Textura t_bicho = r->textura_desde_pixeles(buf, S, S);

    Malla cubo  = r->malla_cubo();
    Malla plano = r->malla_plano();

    V3 cam = { 0.0f, 1.6f, 6.0f };   // 1.6 = altura de los ojos
    float giro = 0.0f;

    uint32_t antes = r->ticks_ms();
    while (!r->poll_quit() && !r->tecla(T3_ESCAPE)) {
        uint32_t ahora = r->ticks_ms();
        float dt = (float)(ahora - antes) / 1000.0f;
        antes = ahora;
        if (dt > 0.1f) dt = 0.1f;    // si el SO nos suspendio, no teletransportar

        // Girar con las flechas, caminar con WASD.
        if (r->tecla(T3_LEFT))  giro -= 2.0f * dt;
        if (r->tecla(T3_RIGHT)) giro += 2.0f * dt;

        float sx = sinf(giro), cz = cosf(giro);
        float vel = 4.0f * dt;
        if (r->tecla(T3_W)) { cam.x += sx*vel; cam.z -= cz*vel; }
        if (r->tecla(T3_S)) { cam.x -= sx*vel; cam.z += cz*vel; }
        if (r->tecla(T3_A)) { cam.x -= cz*vel; cam.z -= sx*vel; }
        if (r->tecla(T3_D)) { cam.x += cz*vel; cam.z += sx*vel; }

        Camara3D c = {
            .pos = cam,
            .mira_a = { cam.x + sx, cam.y, cam.z - cz },
            .fov_grados = 70.0f, .cerca = 0.1f, .lejos = 100.0f
        };

        r->frame_begin(&c, RGB3(20, 18, 30));

        // Piso: un plano estirado. Una sola llamada.
        r->dibujar_malla(plano, (V3){0,0,0}, (V3){0,0,0}, (V3){40,1,40},
                         t_piso, BLANCO);

        // Dos paredes de cubos formando un pasillo.
        for (int i = -6; i <= 6; i++) {
            r->dibujar_malla(cubo, (V3){-4.0f, 1.0f, (float)i*2.0f},
                             (V3){0,0,0}, (V3){2,2,2}, t_muro, BLANCO);
            r->dibujar_malla(cubo, (V3){ 4.0f, 1.0f, (float)i*2.0f},
                             (V3){0,0,0}, (V3){2,2,2}, t_muro, BLANCO);
        }

        // Un cubo que gira en el medio, para ver la rotacion y el z-buffer.
        float t = (float)ahora / 1000.0f;
        r->dibujar_malla(cubo, (V3){0.0f, 1.2f, -6.0f},
                         (V3){t*30.0f, t*45.0f, 0.0f}, (V3){1.2f,1.2f,1.2f},
                         t_muro, RGB3(120,200,255));

        // Los enemigos: billboards. Nunca les decimos hacia donde mirar.
        // Se dibujan en cualquier orden y el z-buffer los resuelve solo.
        for (int i = 0; i < 5; i++) {
            float z = -2.0f - (float)i * 2.5f;
            float x = sinf(t * 0.8f + (float)i) * 2.5f;
            float bob = sinf(t * 3.0f + (float)i) * 0.08f;   // flotan un poco
            r->dibujar_billboard(t_bicho, (V3){x, 0.9f + bob, z},
                                 1.4f, 1.4f, BLANCO);
        }

        r->frame_end();

        if (captura) {
            int fallo = gpu_sdl_capturar_bmp(captura);
            printf(fallo ? "captura FALLO\n" : "captura guardada en %s\n", captura);
            break;
        }
        r->delay_ms(16);   // ~60fps sin quemar la CPU
    }

    r->shutdown();
    printf("listo\n");
    return 0;
}
