// ============================================================
// VimMon — TU JUEGO (starter)
//
// Este es el archivo que vas a hacer TUYO. Ahora mismo hay un
// cuadrado que se mueve con las flechas o WASD y no se sale de
// la pantalla. Borralo y armá lo que quieras: el motor te da
// Entity, World, colisión (entity_overlaps), teclas e imágenes
// de fondo por color. Vos ponés las reglas y la diversión.
// ============================================================

#include "game.h"

// Lógica del jugador: leer teclas y moverse, sin salir del lienzo.
static void player_update(Entity *self, World *w, float dt)
{
    const Renderer *r = w->r;
    const float speed = 300.0f;  // píxeles por segundo

    // El motor te da el World, así podés preguntarle a la
    // plataforma qué teclas están presionadas AHORA.
    if (r->key_down(KEY_LEFT)  || r->key_down(KEY_A)) self->x -= speed * dt;
    if (r->key_down(KEY_RIGHT) || r->key_down(KEY_D)) self->x += speed * dt;
    if (r->key_down(KEY_UP)    || r->key_down(KEY_W)) self->y -= speed * dt;
    if (r->key_down(KEY_DOWN)  || r->key_down(KEY_S)) self->y += speed * dt;

    // Clamp: que no se escape de la ventana.
    int screen_w = r->width();
    int screen_h = r->height();
    if (self->x < 0) self->x = 0;
    if (self->y < 0) self->y = 0;
    if (self->x + self->w > screen_w) self->x = screen_w - self->w;
    if (self->y + self->h > screen_h) self->y = screen_h - self->h;
}

void game_setup(World *w)
{
    Entity *player = world_spawn(w);
    player->x = (float)(w->r->width()  / 2 - 25);
    player->y = (float)(w->r->height() / 2 - 25);
    player->w = 50;
    player->h = 50;
    player->color  = RGB(80, 200, 120);   // verde
    player->update = player_update;       // enchufamos la lógica
    // player->draw queda NULL -> el motor dibuja un rectángulo sólido.
}
