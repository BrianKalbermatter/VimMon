// ============================================================
// VimMon — Ejemplo mínimo del motor
//
// Un rectángulo que se mueve y rebota en los bordes. Sirve para
// dos cosas:
//   1) Probar que el motor completo (framebuffer + entidades + loop)
//      funciona de punta a punta.
//   2) Ser tu PLANTILLA: mirá cómo se crea una entidad y cómo se
//      escribe su update. Tu juego arranca copiando este patrón.
//
// Compilar:
//   make example        (y correr build/hello_entity)
// ============================================================

#include "../engine/engine.h"

#define WIDTH  800
#define HEIGHT 600

// La lógica de ESTA entidad: mover y rebotar contra los bordes.
// Fijate que solo tocamos datos de 'self'. El motor llama a esto
// una vez por frame con dt = segundos desde el frame anterior.
static void bouncer_update(Entity *self, World *w, float dt)
{
    (void)w;  // este ejemplo usa tamaños fijos (WIDTH/HEIGHT)

    // Integración: posición nueva = posición + velocidad * tiempo.
    self->x += self->vx * dt;
    self->y += self->vy * dt;

    // Rebote horizontal: si tocó un borde, invertimos la velocidad
    // en x y reencuadramos para no quedar "pegados" fuera del lienzo.
    if (self->x < 0) {
        self->x = 0;
        self->vx = -self->vx;
    } else if (self->x + self->w > WIDTH) {
        self->x = WIDTH - self->w;
        self->vx = -self->vx;
    }

    // Rebote vertical: misma idea en el eje y.
    if (self->y < 0) {
        self->y = 0;
        self->vy = -self->vy;
    } else if (self->y + self->h > HEIGHT) {
        self->y = HEIGHT - self->h;
        self->vy = -self->vy;
    }
}

int main(void)
{
    // 1) Arrancar el motor con el backend de framebuffer SDL2.
    World *w = engine_init(&sdl_fb_renderer, "VimMon — hello_entity",
                           WIDTH, HEIGHT, RGB(20, 20, 30));
    if (w == NULL)
        return 1;

    // 2) Crear una entidad y configurarla.
    Entity *box = world_spawn(w);
    box->x = 100;  box->y = 100;
    box->w = 60;   box->h = 60;
    box->vx = 220; box->vy = 160;          // píxeles por segundo
    box->color = RGB(230, 80, 60);         // rojo suave
    box->update = bouncer_update;          // enchufamos su lógica
    // box->draw queda en NULL -> el motor dibuja un rectángulo sólido.

    // 3) Correr el loop. Se cierra con la X o con ESC.
    engine_run(w);

    // 4) Cerrar prolijo.
    engine_shutdown(w);
    return 0;
}
