#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdlib.h>
#include <math.h>
#include "ui.h"
#include "pomodoro_bg.h"

#define MAX_COLS 80

typedef struct {
    int x;
    int y;
    int speed;
} Columna;

// Igual que dibujadoTexto pero acepta color
static void
dibujarChar(SDL_Renderer *renderer, TTF_Font *fuente, char c, int x, int y, SDL_Color color)
{
    char txt[2] = {c, '\0'};
    SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente, txt, color);
    if (!sup) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
    SDL_Rect pos = {x, y, sup->w, sup->h};
    SDL_RenderCopy(renderer, tex, NULL, &pos);
    SDL_FreeSurface(sup);
    SDL_DestroyTexture(tex);
}

int
screenMenu(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    int corriendo = 1;
    SDL_Event evento;

    int btn_w = 300;
    int btn_h = 50;
    int btn_x = (ancho - btn_w) / 2;

    // --- Matrix rain setup ---
    srand((unsigned int)SDL_GetTicks());
    Columna cols[MAX_COLS];
    int espaciado = ancho / MAX_COLS;

    for (int i = 0; i < MAX_COLS; i++) {
        cols[i].x     = i * espaciado;
        cols[i].y     = -(rand() % alto);   // arranca en posición random negativa
        cols[i].speed = 2 + rand() % 6;     // velocidad entre 2 y 7
    }

    SDL_Color verde_cabeza = {180, 255, 180, 255};
    SDL_Color verde_cuerpo = {0,   200,  50, 210};
    SDL_Color verde_cola   = {0,   120,  30, 150};

    // --- Título PSEUDOGAMES pulsante en el fondo ---
    // Abrimos la fuente a tamaño grande para el título
    TTF_Font *fuente_titulo = TTF_OpenFont("assets/fonts/main.ttf", 90);
    SDL_Texture *tex_titulo = NULL;
    int titulo_w = 0, titulo_h = 0;
    if (fuente_titulo) {
        SDL_Color blanco = {255, 255, 255, 255};
        SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente_titulo, "PSEUDOGAMES", blanco);
        if (sup) {
            tex_titulo = SDL_CreateTextureFromSurface(renderer, sup);
            titulo_w = sup->w;
            titulo_h = sup->h;
            SDL_FreeSurface(sup);
        }
        TTF_CloseFont(fuente_titulo);
    }
    SDL_SetTextureBlendMode(tex_titulo, SDL_BLENDMODE_BLEND);

    while (corriendo) {
        int clicked = 0, click_x = 0, click_y = 0;

        pom_tick();   // mantener el proceso vivo aunque no se este viendo

        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) return 0;
            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_1: return 1;
                    case SDLK_2: return 2;
                    case SDLK_3: return 3;
                    case SDLK_4: return 4;
                    case SDLK_5: return 5;
                    case SDLK_6: return 6;
                    // control global del pomodoro desde el menu
                    case SDLK_p: pom_send("p", 1); break;
                    case SDLK_0: pom_send("0", 1); break;
                }
            }
            // capturar click dentro del poll, no después
            if (evento.type == SDL_MOUSEBUTTONDOWN &&
                evento.button.button == SDL_BUTTON_LEFT) {
                clicked = 1;
                click_x = evento.button.x;
                click_y = evento.button.y;
            }
        }

        // Fade: rectángulo negro semitransparente en vez de RenderClear total
        // El alpha bajo (25) crea la cola larga de los caracteres automáticamente
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 25);
        SDL_RenderFillRect(renderer, NULL);

        // --- Dibujar columnas de 0s y 1s ---
        for (int i = 0; i < MAX_COLS; i++) {
            char c = (rand() % 2) ? '0' : '1';

            // cabeza: más brillante
            dibujarChar(renderer, fuente, c, cols[i].x, cols[i].y, verde_cabeza);

            // cuerpo: un char atrás
            if (cols[i].y - 16 >= 0) {
                char c2 = (rand() % 2) ? '0' : '1';
                dibujarChar(renderer, fuente, c2, cols[i].x, cols[i].y - 16, verde_cuerpo);
            }

            // cola: dos chars atrás
            if (cols[i].y - 32 >= 0) {
                char c3 = (rand() % 2) ? '0' : '1';
                dibujarChar(renderer, fuente, c3, cols[i].x, cols[i].y - 32, verde_cola);
            }

            cols[i].y += cols[i].speed;

            // reset: cuando sale por abajo vuelve a aparecer arriba con delay random
            if (cols[i].y > alto) {
                cols[i].y     = -(rand() % 300);
                cols[i].speed = 2 + rand() % 6;
            }
        }

        // --- Título PSEUDOGAMES pulsante en el fondo ---
        if (tex_titulo) {
            float t     = SDL_GetTicks() / 1000.0f;
            // oscila entre 0.85 y 1.15 — suave y lento
            float scale = 1.0f + 0.15f * sinf(t * 0.6f);
            int w = (int)(titulo_w * scale);
            int h = (int)(titulo_h * scale);
            int x = (ancho - w) / 2;
            int y = alto / 6 - h / 2;    // parte superior, centrado
            SDL_SetTextureAlphaMod(tex_titulo, 35); // muy transparente: empotrado en el fondo
            SDL_Rect dst = {x, y, w, h};
            SDL_RenderCopy(renderer, tex_titulo, NULL, &dst);
        }

        // --- Botones encima del efecto ---
        int mx, my;
        SDL_GetMouseState(&mx, &my);
        char *labels[7] = {"Jugar", "DOC", "Pomodoro", "Editor Libre",
                           "Seleccion de Nivel", "Soluciones", "Salir"};

        for (int i = 0; i < 7; i++) {
            int btn_y = (alto / 2) - 80 + (i * 70);
            int hover = (mx >= btn_x && mx <= btn_x + btn_w &&
                         my >= btn_y && my <= btn_y + btn_h);

            SDL_Rect btn = {btn_x, btn_y, btn_w, btn_h};

            // Fondo acrílico: azul semitransparente, se ve la lluvia por detrás
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            if (hover)
                SDL_SetRenderDrawColor(renderer, 0, 150, 255, 80);
            else
                SDL_SetRenderDrawColor(renderer, 0, 60, 120, 60);
            SDL_RenderFillRect(renderer, &btn);

            // Borde del botón
            if (hover)
                SDL_SetRenderDrawColor(renderer, 0, 220, 255, 220);
            else
                SDL_SetRenderDrawColor(renderer, 0, 180, 100, 160);
            SDL_RenderDrawRect(renderer, &btn);

            // Texto centrado dentro del botón
            int txt_w, txt_h;
            TTF_SizeUTF8(fuente, labels[i], &txt_w, &txt_h);
            int txt_x = btn_x + (btn_w - txt_w) / 2;
            int txt_y = btn_y + (btn_h - txt_h) / 2;

            SDL_Color color_txt = hover ? (SDL_Color){255, 255, 255, 255}
                                        : (SDL_Color){180, 255, 180, 255};
            SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente, labels[i], color_txt);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
            SDL_Rect pos_txt = {txt_x, txt_y, sup->w, sup->h};
            SDL_RenderCopy(renderer, tex, NULL, &pos_txt);
            SDL_FreeSurface(sup);
            SDL_DestroyTexture(tex);

            if (clicked &&
                click_x >= btn_x && click_x <= btn_x + btn_w &&
                click_y >= btn_y && click_y <= btn_y + btn_h)
                return (i == 6) ? 0 : i + 1;
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 fps
    }

    if (tex_titulo) SDL_DestroyTexture(tex_titulo);
    return 0;
}
