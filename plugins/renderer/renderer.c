#include "../../bus/plugin.h"
#include <stdio.h>

/*
  |  Esta es la regla de oro en C — exponé lo mínimo indispensable. Todo lo que sea
  ▎ detalle interno → static. Solo lo que el resto del sistema necesita tocar → público. Es el
  ▎ equivalente a private / public que en otros lenguajes te viene de fábrica, pero en C lo controlás
  ▎ vos con static. Es así de fácil.
 */
static int  renderer_init(void)
{
    printf("[renderer] iniciado\n");
    return 0;
}
static void renderer_shutdown(void)      { printf("[renderer] apagado\n"); }
static void renderer_tick(float delta)   { (void)delta; }
static void renderer_on_event(Event *e)  { (void)e; }

Plugin renderer_plugin = {
    .name      = "renderer",
    .version   = "0.1",
    .init      = renderer_init,
    .shutdown  = renderer_shutdown,
    .tick      = renderer_tick,
    .on_event  = renderer_on_event,
};
