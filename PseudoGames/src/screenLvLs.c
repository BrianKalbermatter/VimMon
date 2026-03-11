#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "niveles.h"
#include "progreso.h"
#include "pomodoro_bg.h"

int
screenLvLs(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto){
    (void)alto;
    SDL_Event evento;
    int total = total_niveles();

    // --- layout de tarjetas ---
    int cols    = 3;
    int card_w  = 200;
    int card_h  = 150;
    int gap_x   = 30;
    int gap_y   = 25;
    int grid_w  = cols * card_w + (cols - 1) * gap_x;
    int start_x = (ancho - grid_w) / 2;
    int start_y = 80;

    while (1) {
        int clicked = 0, click_x = 0, click_y = 0;

        pom_tick();

        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) return 0;
            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_ESCAPE: return 0;
                    case SDLK_p: pom_send("p", 1); break;
                    case SDLK_0: pom_send("0", 1); break;
                    default: break;
                }
            }
            if (evento.type == SDL_MOUSEBUTTONDOWN &&
                evento.button.button == SDL_BUTTON_LEFT) {
                clicked  = 1;
                click_x  = evento.button.x;
                click_y  = evento.button.y;
            }
        }

        SDL_SetRenderDrawColor(renderer, 15, 15, 22, 255);
        SDL_RenderClear(renderer);

        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // titulo de la pantalla
        SDL_Color c_titulo = {180, 180, 200, 255};
        dibujadoTextoColor(renderer, fuente, "Selecciona un nivel", 0, 18, c_titulo);

        for (int i = 0; i < total; i++) {
            Nivel *nv = obtener_nivel(i + 1);
            if (!nv) continue;

            int num         = i + 1;
            int col         = i % cols;
            int row         = i / cols;
            int cx          = start_x + col * (card_w + gap_x);
            int cy          = start_y + row * (card_h + gap_y);
            int desbloqueado = nivel_desbloqueado(num);
            int completado   = esta_completado(num);
            int hover        = desbloqueado &&
                               mx >= cx && mx <= cx + card_w &&
                               my >= cy && my <= cy + card_h;

            SDL_Rect card = {cx, cy, card_w, card_h};

            // --- color de fondo segun estado ---
            if (!desbloqueado) {
                SDL_SetRenderDrawColor(renderer, 38, 38, 42, 255);
            } else if (completado) {
                SDL_SetRenderDrawColor(renderer,
                    hover ? 10 : 8,
                    hover ? 90 : 70,
                    hover ? 45 : 35, 255);
            } else {
                SDL_SetRenderDrawColor(renderer,
                    hover ? 25 : 18,
                    hover ? 60 : 40,
                    hover ? 120 : 90, 255);
            }
            SDL_RenderFillRect(renderer, &card);

            // borde superior de color
            SDL_Rect borde = {cx, cy, card_w, 4};
            if (!desbloqueado)
                SDL_SetRenderDrawColor(renderer, 70, 70, 75, 255);
            else if (completado)
                SDL_SetRenderDrawColor(renderer, 60, 220, 100, 255);
            else
                SDL_SetRenderDrawColor(renderer, 60, 130, 255, 255);
            SDL_RenderFillRect(renderer, &borde);

            // --- textos dentro de la tarjeta ---
            SDL_Color c_num, c_titulo_card, c_pts;

            if (!desbloqueado) {
                c_num        = (SDL_Color){70,  70,  75,  255};
                c_titulo_card= (SDL_Color){70,  70,  75,  255};
                c_pts        = (SDL_Color){60,  60,  65,  255};
            } else if (completado) {
                c_num        = (SDL_Color){80,  220, 110, 255};
                c_titulo_card= (SDL_Color){200, 255, 210, 255};
                c_pts        = (SDL_Color){60,  180, 90,  255};
            } else {
                c_num        = (SDL_Color){100, 160, 255, 255};
                c_titulo_card= (SDL_Color){220, 230, 255, 255};
                c_pts        = (SDL_Color){80,  120, 200, 255};
            }

            // "Nivel N"
            char str_num[32];
            snprintf(str_num, sizeof(str_num), "Nivel %d", num);
            dibujadoTextoColor(renderer, fuente, str_num, cx, cy + 8, c_num);

            // nombre del nivel
            dibujadoTextoColor(renderer, fuente, nv->titulo, cx, cy + 38, c_titulo_card);

            // estado: OK o BLOQUEADO
            if (completado)
                dibujadoTextoColor(renderer, fuente, "[OK]", cx, cy + 68, c_num);
            else if (!desbloqueado)
                dibujadoTextoColor(renderer, fuente, "[BLOQUEADO]", cx, cy + 68, c_num);

            // puntos
            dibujadoTextoColor(renderer, fuente, "100 pts", cx, cy + card_h - 38, c_pts);

            // click
            if (desbloqueado && clicked &&
                click_x >= cx && click_x <= cx + card_w &&
                click_y >= cy && click_y <= cy + card_h)
                return num;
        }

        // --- boton resetear progreso ---
        int rows_used = (total + cols - 1) / cols;
        int rbtn_w = 220, rbtn_h = 36;
        int rbtn_x = (ancho - rbtn_w) / 2;
        int rbtn_y = start_y + rows_used * (card_h + gap_y) + 10;
        int rhover = mx >= rbtn_x && mx <= rbtn_x + rbtn_w &&
                     my >= rbtn_y && my <= rbtn_y + rbtn_h;

        SDL_SetRenderDrawColor(renderer,
            rhover ? 160 : 100, rhover ? 20 : 12, rhover ? 20 : 12, 255);
        SDL_Rect rbtn = {rbtn_x, rbtn_y, rbtn_w, rbtn_h};
        SDL_RenderFillRect(renderer, &rbtn);
        SDL_Color c_reset = {255, 160, 160, 255};
        dibujadoTextoColor(renderer, fuente, "Resetear progreso", rbtn_x, rbtn_y, c_reset);

        if (clicked &&
            click_x >= rbtn_x && click_x <= rbtn_x + rbtn_w &&
            click_y >= rbtn_y && click_y <= rbtn_y + rbtn_h)
            resetear_progreso();

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}
