#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "niveles.h"
#include "progreso.h"
#include "pomodoro_bg.h"
#include "audio.h"

/* Dibuja una estrella de 5 puntas rellena centrada en (cx, cy) con radio r.
   Usa scanline fill sobre el poligono de 10 vertices (5 ext + 5 int). */
static void
dibujar_estrella(SDL_Renderer *renderer, int cx, int cy, int r, SDL_Color color)
{
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);

    float pts[10][2];
    float ri = r * 0.42f;   /* radio interior */

    for (int i = 0; i < 5; i++) {
        float ao = (float)(-M_PI / 2.0 + i * 2.0 * M_PI / 5.0);
        float ai = ao + (float)(M_PI / 5.0);
        pts[i*2  ][0] = cx + cosf(ao) * r;
        pts[i*2  ][1] = cy + sinf(ao) * r;
        pts[i*2+1][0] = cx + cosf(ai) * ri;
        pts[i*2+1][1] = cy + sinf(ai) * ri;
    }

    /* scanline fill */
    for (int y = cy - r; y <= cy + r; y++) {
        float xs[20];
        int   nx = 0;
        for (int i = 0; i < 10; i++) {
            int   j  = (i + 1) % 10;
            float y0 = pts[i][1], y1 = pts[j][1];
            float x0 = pts[i][0], x1 = pts[j][0];
            if ((y0 <= y && y1 > y) || (y1 <= y && y0 > y))
                xs[nx++] = x0 + ((float)y - y0) / (y1 - y0) * (x1 - x0);
        }
        /* ordenar intersecciones (burbuja, max 10 elementos) */
        for (int a = 0; a < nx - 1; a++)
            for (int b = a + 1; b < nx; b++)
                if (xs[a] > xs[b]) { float t = xs[a]; xs[a] = xs[b]; xs[b] = t; }
        for (int a = 0; a + 1 < nx; a += 2)
            SDL_RenderDrawLine(renderer, (int)xs[a], y, (int)xs[a+1], y);
    }
}

/* Dibuja N estrellas seguidas desde el punto (x, y) */
static void
dibujar_estrellas(SDL_Renderer *renderer, int n, int x, int y, int r, SDL_Color color)
{
    int gap = r * 2 + 5;
    for (int i = 0; i < n; i++)
        dibujar_estrella(renderer, x + i * gap + r, y, r, color);
}

/* Dibuja un candado centrado en (lx, ly) */
static void
dibujar_candado(SDL_Renderer *renderer, int lx, int ly)
{
    SDL_Color c = {75, 78, 85, 255};
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);

    /* argolla: dos laterales + techo */
    SDL_Rect sl  = {lx-10, ly-22, 4, 24};   /* lado izq  */
    SDL_Rect sr  = {lx+ 6, ly-22, 4, 24};   /* lado der  */
    SDL_Rect tch = {lx-10, ly-22, 20, 4};   /* techo     */
    SDL_RenderFillRect(renderer, &sl);
    SDL_RenderFillRect(renderer, &sr);
    SDL_RenderFillRect(renderer, &tch);

    /* cuerpo */
    SDL_Rect body = {lx-16, ly, 32, 24};
    SDL_RenderFillRect(renderer, &body);

    /* ojo del candado */
    SDL_SetRenderDrawColor(renderer, 38, 38, 42, 255);
    SDL_Rect ojo = {lx-4, ly+6, 8, 12};
    SDL_RenderFillRect(renderer, &ojo);
}

int
screenLvLs(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    SDL_Event evento;
    int total = total_niveles();

    /* layout de tarjetas */
    int cols   = 3;
    int card_w = 200;
    int card_h = 150;
    int gap_x  = 30;
    int gap_y  = 25;
    int grid_w = cols * card_w + (cols - 1) * gap_x;
    int start_x = (ancho - grid_w) / 2;
    int start_y = 80;

    int rows_total = (total + cols - 1) / cols;
    int rbtn_h     = 36;
    /* altura total del contenido */
    int contenido_h = start_y + rows_total * (card_h + gap_y) + rbtn_h + 40;

    int scroll_y   = 0;
    int scroll_max = 0;   /* se recalcula cada frame */

    while (1) {
        int clicked = 0, click_x = 0, click_y = 0;
        pom_tick();

        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) return 0;
            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_ESCAPE: return 0;
                    case SDLK_UP:
                        scroll_y -= 40;
                        if (scroll_y < 0) scroll_y = 0;
                        break;
                    case SDLK_DOWN:
                        scroll_y += 40;
                        break;
                    case SDLK_p: pom_send("p", 1); break;
                    case SDLK_0: pom_send("0", 1); break;
                    default: break;
                }
            }
            if (evento.type == SDL_MOUSEWHEEL) {
                scroll_y -= evento.wheel.y * 45;  /* y>0 = rueda arriba */
                if (scroll_y < 0) scroll_y = 0;
            }
            if (evento.type == SDL_MOUSEBUTTONDOWN &&
                evento.button.button == SDL_BUTTON_LEFT) {
                clicked = 1;
                click_x = evento.button.x;
                click_y = evento.button.y;
            }
        }

        /* recalcular clamp del scroll */
        scroll_max = contenido_h - alto;
        if (scroll_max < 0) scroll_max = 0;
        if (scroll_y > scroll_max) scroll_y = scroll_max;

        SDL_SetRenderDrawColor(renderer, 15, 15, 22, 255);
        SDL_RenderClear(renderer);

        /* clip: no dibujar fuera de la pantalla */
        SDL_Rect viewport = {0, 0, ancho, alto};
        SDL_RenderSetClipRect(renderer, &viewport);

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        /* titulo fijo arriba (no scrollea) */
        SDL_Color c_titulo = {180, 180, 200, 255};
        dibujadoTextoColor(renderer, fuente, "Selecciona un nivel", 0, 18, c_titulo);

        for (int i = 0; i < total; i++) {
            Nivel *nv = obtener_nivel(i + 1);
            if (!nv) continue;

            int num   = i + 1;
            int col   = i % cols;
            int row   = i / cols;
            int cx    = start_x + col * (card_w + gap_x);
            int cy    = start_y + row  * (card_h + gap_y) - scroll_y;

            /* saltar tarjetas completamente fuera de pantalla */
            if (cy + card_h < 0 || cy > alto) continue;

            int desbloqueado = nivel_desbloqueado(num);
            int completado   = esta_completado(num);
            int hover        = desbloqueado &&
                               mx >= cx && mx <= cx + card_w &&
                               my >= cy && my <= cy + card_h;

            SDL_Rect card = {cx, cy, card_w, card_h};

            /* fondo de la tarjeta */
            if (!desbloqueado) {
                SDL_SetRenderDrawColor(renderer, 28, 28, 32, 255);
            } else if (completado) {
                SDL_SetRenderDrawColor(renderer,
                    hover ? 10 : 8, hover ? 90 : 70, hover ? 45 : 35, 255);
            } else {
                SDL_SetRenderDrawColor(renderer,
                    hover ? 25 : 18, hover ? 60 : 40, hover ? 120 : 90, 255);
            }
            SDL_RenderFillRect(renderer, &card);

            /* borde superior de color */
            SDL_Rect borde = {cx, cy, card_w, 4};
            if (!desbloqueado)
                SDL_SetRenderDrawColor(renderer, 55, 55, 60, 255);
            else if (completado)
                SDL_SetRenderDrawColor(renderer, 60, 220, 100, 255);
            else
                SDL_SetRenderDrawColor(renderer, 60, 130, 255, 255);
            SDL_RenderFillRect(renderer, &borde);

            if (!desbloqueado) {
                /* ── Nivel bloqueado: solo candado centrado ── */
                dibujar_candado(renderer, cx + card_w / 2, cy + card_h / 2);

                /* numero de nivel en gris muy dim */
                char str_num[32];
                snprintf(str_num, sizeof(str_num), "Nivel %d", num);
                SDL_Color c_dim = {50, 50, 55, 255};
                dibujadoTextoColor(renderer, fuente, str_num, cx, cy + 8, c_dim);

            } else {
                /* ── Nivel desbloqueado: contenido normal ── */
                SDL_Color c_num, c_tit, c_pts;
                if (completado) {
                    c_num = (SDL_Color){80,  220, 110, 255};
                    c_tit = (SDL_Color){200, 255, 210, 255};
                    c_pts = (SDL_Color){60,  180,  90, 255};
                } else {
                    c_num = (SDL_Color){100, 160, 255, 255};
                    c_tit = (SDL_Color){220, 230, 255, 255};
                    c_pts = (SDL_Color){80,  120, 200, 255};
                }

                char str_num[32];
                snprintf(str_num, sizeof(str_num), "Nivel %d", num);
                dibujadoTextoColor(renderer, fuente, str_num,    cx, cy +  8, c_num);
                dibujadoTextoColor(renderer, fuente, nv->titulo, cx, cy + 38, c_tit);

                if (completado)
                    dibujadoTextoColor(renderer, fuente, "[OK]", cx, cy + 68, c_num);

                /* estrellas de dificultad dibujadas con SDL */
                int estrellas = (int)(strlen(nv->dificultad) / 3);
                if (estrellas < 1) estrellas = 1;
                SDL_Color c_dif;
                if      (estrellas <= 1) c_dif = (SDL_Color){ 80, 220, 100, 255};
                else if (estrellas == 2) c_dif = (SDL_Color){255, 200,  50, 255};
                else if (estrellas == 3) c_dif = (SDL_Color){255, 130,  20, 255};
                else                    c_dif = (SDL_Color){255,  50,  50, 255};
                SDL_Color c_lbl = {100, 100, 120, 255};
                int dif_y = cy + card_h - 90;
                dibujadoTextoColor(renderer, fuente, "Dificultad:",
                                   cx, dif_y, c_lbl);
                int lbl_w2 = 0;
                TTF_SizeUTF8(fuente, "Dificultad: ", &lbl_w2, NULL);
                dibujar_estrellas(renderer, estrellas,
                                  cx + lbl_w2 + 10, dif_y + 20, 6, c_dif);

                dibujadoTextoColor(renderer, fuente, "100 pts",
                                   cx, cy + card_h - 38, c_pts);

                /* click */
                if (clicked &&
                    click_x >= cx && click_x <= cx + card_w &&
                    click_y >= cy && click_y <= cy + card_h) {
                    audio_sfx_btn();
                    return num;
                }
            }
        }

        /* boton resetear progreso */
        int rbtn_w = 220;
        int rbtn_x = (ancho - rbtn_w) / 2;
        int rbtn_y = start_y + rows_total * (card_h + gap_y) + 10 - scroll_y;

        if (rbtn_y + rbtn_h >= 0 && rbtn_y <= alto) {
            int rhover = mx >= rbtn_x && mx <= rbtn_x + rbtn_w &&
                         my >= rbtn_y && my <= rbtn_y + rbtn_h;
            SDL_SetRenderDrawColor(renderer,
                rhover ? 160 : 100, rhover ? 20 : 12, rhover ? 20 : 12, 255);
            SDL_Rect rbtn = {rbtn_x, rbtn_y, rbtn_w, rbtn_h};
            SDL_RenderFillRect(renderer, &rbtn);
            SDL_Color c_reset = {255, 160, 160, 255};
            dibujadoTextoColor(renderer, fuente, "Resetear progreso",
                               rbtn_x, rbtn_y, c_reset);

            if (clicked &&
                click_x >= rbtn_x && click_x <= rbtn_x + rbtn_w &&
                click_y >= rbtn_y && click_y <= rbtn_y + rbtn_h) {
                audio_sfx_btn();
                resetear_progreso();
            }
        }

        /* barra de scroll indicadora (derecha) */
        if (scroll_max > 0) {
            int bar_x = ancho - 6;
            int bar_h = alto * alto / contenido_h;
            if (bar_h < 20) bar_h = 20;
            int bar_y = (int)((long long)scroll_y * (alto - bar_h) / scroll_max);
            SDL_SetRenderDrawColor(renderer, 60, 60, 80, 200);
            SDL_Rect bar_bg = {bar_x, 0, 5, alto};
            SDL_RenderFillRect(renderer, &bar_bg);
            SDL_SetRenderDrawColor(renderer, 120, 120, 160, 255);
            SDL_Rect bar = {bar_x, bar_y, 5, bar_h};
            SDL_RenderFillRect(renderer, &bar);
        }

        SDL_RenderSetClipRect(renderer, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}
