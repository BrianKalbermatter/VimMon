#include "../../bus/plugin.h"
#include "renderer.h"
#include "../../engine/engine.h"
#include "../../game/game.h"
#include <stdio.h>

/*
  Este plugin es el PUENTE entre el bus y el motor gráfico.
  No dibuja él mismo: cuando el bus le manda EVENT_RENDER_FRAME
  (por ejemplo, al escribir "engine" en la consola), arranca el
  motor, monta tu juego (game_setup) y le cede el control al
  game loop hasta que cerrás la ventana.

  Regla de oro en C: exponé lo mínimo. Todo lo interno → static.
  Solo renderer_plugin es público.
*/

#define RENDER_WIDTH  800
#define RENDER_HEIGHT 600

// Abre la ventana, monta el juego y corre el loop (bloquea hasta cerrar).
static void launch_engine(void)
{
    World *w = engine_init(&sdl_fb_renderer, "VimMon — engine",
                           RENDER_WIDTH, RENDER_HEIGHT, RGB(20, 20, 30));
    if (w == NULL) {
        printf("[renderer] no se pudo iniciar el motor\n");
        return;
    }

    game_setup(w);        // tu juego (game/game.c)
    engine_run(w);        // loop hasta X o ESC
    engine_shutdown(w);   // cierra la ventana, vuelve a la consola
    printf("[renderer] ventana cerrada, de vuelta en vimmon\n");
}

static int  renderer_init(void)        { printf("[renderer] iniciado\n"); return 0; }
static void renderer_shutdown(void)    { printf("[renderer] apagado\n"); }
static void renderer_tick(float delta) { (void)delta; }

static void renderer_on_event(Event *e)
{
    switch (e->type) {
    case EVENT_RENDER_FRAME:
        launch_engine();
        break;
    case EVENT_SCENE_UPDATE:
        // Futuro: acá se va a redibujar la escena PAED (SceneState).
        break;
    default:
        break;
    }
}

Plugin renderer_plugin = {
    .name      = "renderer",
    .version   = "0.2",
    .init      = renderer_init,
    .shutdown  = renderer_shutdown,
    .tick      = renderer_tick,
    .on_event  = renderer_on_event,
};
