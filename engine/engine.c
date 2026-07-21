// ============================================================
// VimMon — ENGINE 2D (implementación)
// Ver engine.h para el contrato y la filosofía.
// ============================================================

#include "engine.h"

// Un único mundo estático: sin malloc, ideal para aprender y para el
// futuro bare-metal (FASE 6), donde no hay heap todavía.
static World g_world;

World *engine_init(const Renderer *r, const char *title,
                   int width, int height, uint32_t clear_color)
{
    if (r == NULL)
        return NULL;

    if (r->init(title, width, height) != 0)
        return NULL;

    g_world.r           = r;
    g_world.count       = 0;
    g_world.clear_color = clear_color;
    g_world.running     = 1;

    // Todas las ranuras arrancan muertas.
    for (int i = 0; i < ENGINE_MAX_ENTITIES; i++)
        g_world.entities[i].alive = 0;

    return &g_world;
}

Entity *world_spawn(World *w)
{
    if (w == NULL || w->count >= ENGINE_MAX_ENTITIES)
        return NULL;

    Entity *e = &w->entities[w->count];
    w->count++;

    // Dejamos la entidad en cero y viva. El que hace el juego
    // completa los campos que le interesen.
    *e = (Entity){0};
    e->alive = 1;
    return e;
}

int entity_overlaps(const Entity *a, const Entity *b)
{
    // AABB: NO se solapan si uno está enteramente a un lado del otro.
    // Negamos esa condición para saber cuándo SÍ chocan.
    if (a->x + a->w <= b->x) return 0;   // a a la izquierda de b
    if (b->x + b->w <= a->x) return 0;   // b a la izquierda de a
    if (a->y + a->h <= b->y) return 0;   // a arriba de b
    if (b->y + b->h <= a->y) return 0;   // b arriba de a
    return 1;
}

// Dibujo por defecto cuando la entidad no trae su propio draw:
// un rectángulo sólido con su color.
static void draw_default(const Renderer *r, const Entity *e)
{
    r->fill_rect((int)e->x, (int)e->y, e->w, e->h, e->color);
}

void engine_run(World *w)
{
    if (w == NULL)
        return;

    const Renderer *r = w->r;
    uint32_t prev = r->ticks_ms();

    // El game loop. No sabemos cuántas vueltas dará (depende del
    // usuario) -> while, no for. Corta por un evento externo.
    while (w->running) {
        // 1) Delta de tiempo: segundos desde el frame anterior.
        uint32_t now = r->ticks_ms();
        float dt = (float)(now - prev) / 1000.0f;
        prev = now;

        // 2) Entrada: cerrar por la X o por ESC.
        if (r->poll_quit() || r->key_down(KEY_ESCAPE)) {
            w->running = 0;
            break;
        }

        // 3) Lógica: la de cada entidad viva (tu código).
        for (int i = 0; i < w->count; i++) {
            Entity *e = &w->entities[i];
            if (e->alive && e->update)
                e->update(e, w, dt);
        }

        // 4) Dibujo: fondo limpio y cada entidad viva encima.
        r->clear(w->clear_color);
        for (int i = 0; i < w->count; i++) {
            Entity *e = &w->entities[i];
            if (!e->alive)
                continue;
            if (e->draw)
                e->draw(e, r);
            else
                draw_default(r, e);
        }

        // 5) Mostrar el frame y ceder CPU (~60 FPS).
        r->present();
        r->delay_ms(16);
    }
}

void engine_shutdown(World *w)
{
    if (w != NULL && w->r != NULL)
        w->r->shutdown();
}
