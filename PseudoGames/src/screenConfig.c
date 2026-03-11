#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "ui.h"

// Opciones de configuracion disponibles
typedef struct {
    const char *nombre;
    const char *valores[4];  // posibles valores
    int n_valores;
    int seleccion;           // indice del valor actual
} OpcionConfig;

int
screenConfig(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto,
             SDL_Window *ventana)
{
    // --- definir opciones ---
    OpcionConfig opciones[] = {
        { "Pantalla completa",  {"Si", "No"},       2, 0 },
        { "Velocidad lluvia",   {"Lenta", "Normal", "Rapida"}, 3, 1 },
        { "Brillo",             {"Bajo", "Normal", "Alto"},    3, 1 },
    };
    int n_opciones = (int)(sizeof(opciones) / sizeof(opciones[0]));
    int seleccionada = 0;  // fila seleccionada con el teclado

    SDL_Event evento;
    int corriendo = 1;

    while (corriendo) {
        int clicked = 0, click_x = 0, click_y = 0;

        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT)   return 0;
            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_ESCAPE: corriendo = 0; break;
                    case SDLK_UP:
                        seleccionada--;
                        if (seleccionada < 0) seleccionada = n_opciones - 1;
                        break;
                    case SDLK_DOWN:
                        seleccionada++;
                        if (seleccionada >= n_opciones) seleccionada = 0;
                        break;
                    case SDLK_LEFT:
                        opciones[seleccionada].seleccion--;
                        if (opciones[seleccionada].seleccion < 0)
                            opciones[seleccionada].seleccion = opciones[seleccionada].n_valores - 1;
                        break;
                    case SDLK_RIGHT: case SDLK_RETURN:
                        opciones[seleccionada].seleccion++;
                        if (opciones[seleccionada].seleccion >= opciones[seleccionada].n_valores)
                            opciones[seleccionada].seleccion = 0;
                        break;
                    default: break;
                }

                // aplicar pantalla completa en tiempo real
                if (opciones[0].seleccion == 0)
                    SDL_SetWindowFullscreen(ventana, SDL_WINDOW_FULLSCREEN_DESKTOP);
                else
                    SDL_SetWindowFullscreen(ventana, 0);
            }
            if (evento.type == SDL_MOUSEBUTTONDOWN &&
                evento.button.button == SDL_BUTTON_LEFT) {
                clicked  = 1;
                click_x  = evento.button.x;
                click_y  = evento.button.y;
            }
        }

        // --- render ---
        SDL_SetRenderDrawColor(renderer, 12, 12, 18, 255);
        SDL_RenderClear(renderer);

        // panel central
        int panel_w = 520, panel_h = 400;
        int panel_x = (ancho - panel_w) / 2;
        int panel_y = (alto  - panel_h) / 2;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 20, 20, 35, 220);
        SDL_Rect panel = {panel_x, panel_y, panel_w, panel_h};
        SDL_RenderFillRect(renderer, &panel);
        SDL_SetRenderDrawColor(renderer, 60, 60, 100, 255);
        SDL_RenderDrawRect(renderer, &panel);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        // titulo
        SDL_Color c_titulo = {180, 180, 220, 255};
        dibujadoTextoColor(renderer, fuente, "Configuracion",
            panel_x + 20, panel_y + 16, c_titulo);

        // linea divisoria bajo el titulo
        SDL_SetRenderDrawColor(renderer, 60, 60, 100, 255);
        SDL_RenderDrawLine(renderer,
            panel_x + 16, panel_y + 48,
            panel_x + panel_w - 16, panel_y + 48);

        // opciones
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        for (int i = 0; i < n_opciones; i++) {
            int oy = panel_y + 80 + i * 100;
            int hover_row = (mx >= panel_x + 16 && mx <= panel_x + panel_w - 16 &&
                             my >= oy - 10   && my <= oy + 60);

            // highlight de fila
            if (i == seleccionada || hover_row) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 40, 40, 80, 120);
                SDL_Rect row_bg = {panel_x + 12, oy - 10,
                                   panel_w - 24, 62};
                SDL_RenderFillRect(renderer, &row_bg);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                if (hover_row) seleccionada = i;
            }

            // nombre de la opcion
            SDL_Color c_nombre = (i == seleccionada)
                ? (SDL_Color){220, 220, 255, 255}
                : (SDL_Color){140, 140, 160, 255};
            dibujadoTextoColor(renderer, fuente, opciones[i].nombre,
                panel_x + 20, oy, c_nombre);

            // valores como botones < valor >
            int vx = panel_x + panel_w - 200;
            SDL_Color c_flecha = {80, 120, 255, 255};
            SDL_Color c_valor  = {200, 255, 200, 255};

            dibujadoTextoColor(renderer, fuente, "<", vx, oy, c_flecha);

            const char *val_actual = opciones[i].valores[opciones[i].seleccion];
            int vw; TTF_SizeUTF8(fuente, val_actual, &vw, NULL);
            dibujadoTextoColor(renderer, fuente, val_actual,
                vx + 30 + (120 - vw) / 2, oy, c_valor);

            dibujadoTextoColor(renderer, fuente, ">", vx + 160, oy, c_flecha);

            // click en < o >
            if (clicked) {
                // boton <
                if (click_x >= vx && click_x <= vx + 20 &&
                    click_y >= oy && click_y <= oy + 24) {
                    opciones[i].seleccion--;
                    if (opciones[i].seleccion < 0)
                        opciones[i].seleccion = opciones[i].n_valores - 1;
                }
                // boton >
                if (click_x >= vx + 150 && click_x <= vx + 180 &&
                    click_y >= oy && click_y <= oy + 24) {
                    opciones[i].seleccion++;
                    if (opciones[i].seleccion >= opciones[i].n_valores)
                        opciones[i].seleccion = 0;
                }
                // aplicar pantalla completa
                if (opciones[0].seleccion == 0)
                    SDL_SetWindowFullscreen(ventana, SDL_WINDOW_FULLSCREEN_DESKTOP);
                else
                    SDL_SetWindowFullscreen(ventana, 0);
            }
        }

        // boton feedback fuera del panel, centrado abajo
        int fb_w = 130, fb_h = 28;
        int fb_x = (ancho - fb_w) / 2;
        int fb_y = panel_y + panel_h + 18;
        int fb_hover = (mx >= fb_x && mx <= fb_x + fb_w &&
                        my >= fb_y && my <= fb_y + fb_h);

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer,
            fb_hover ? 60 : 30, fb_hover ? 45 : 22, fb_hover ? 120 : 60, 200);
        SDL_Rect fb_btn = {fb_x, fb_y, fb_w, fb_h};
        SDL_RenderFillRect(renderer, &fb_btn);
        SDL_SetRenderDrawColor(renderer,
            fb_hover ? 160 : 80, fb_hover ? 120 : 60, fb_hover ? 255 : 160, 255);
        SDL_RenderDrawRect(renderer, &fb_btn);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

        SDL_Color c_fb = fb_hover ? (SDL_Color){220,200,255,255}
                                  : (SDL_Color){130,110,200,255};
        int fbw, fbh; TTF_SizeUTF8(fuente, "Feedback", &fbw, &fbh);
        // dibujadoTextoColor aplica +10 en x y +12 en y internamente, hay que compensarlo
        dibujadoTextoColor(renderer, fuente, "Feedback",
            fb_x + (fb_w - fbw) / 2 - 10, fb_y + (fb_h - fbh) / 2 - 12, c_fb);

        if (clicked &&
            click_x >= fb_x && click_x <= fb_x + fb_w &&
            click_y >= fb_y && click_y <= fb_y + fb_h)
            screenFeedback(renderer, fuente, ancho, alto);

        // ayuda abajo a la izquierda
        SDL_Color c_help = {70, 70, 90, 255};
        dibujadoTextoColor(renderer, fuente,
            "[flechas] navegar   [</>] cambiar   [ESC] volver",
            panel_x + 20, panel_y + panel_h - 34, c_help);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    return 0;
}
