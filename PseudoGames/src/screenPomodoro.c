#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "pomodoro_bg.h"

#ifdef _WIN32
// ── Pomodoro nativo SDL2 para Windows (sin bash/forkpty) ─────────────────
int
screenPomodoro(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    int TRABAJO_SEG  = 25 * 60;
    int DESCANSO_SEG =  5 * 60;

    int     fase     = 0;             // 0=TRABAJO  1=DESCANSO
    int     segundos = TRABAJO_SEG;
    int     pausado  = 0;
    int     ciclos   = 0;
    Uint32  ultimo   = SDL_GetTicks();

    SDL_Color c_verde   = {  0, 220,  80, 255};
    SDL_Color c_fondo   = {  0,  30,  10, 255};
    SDL_Color c_amarillo= {220, 200,   0, 255};
    SDL_Color c_gris    = {100, 100, 100, 255};

    int corriendo = 1;
    SDL_Event evento;

    while (corriendo) {

        // tick cada segundo si no esta pausado
        Uint32 ahora = SDL_GetTicks();
        if (!pausado && ahora - ultimo >= 1000) {
            ultimo += 1000;
            segundos--;
            if (segundos < 0) {
                fase = 1 - fase;
                if (fase == 0) ciclos++;
                segundos = (fase == 0) ? TRABAJO_SEG : DESCANSO_SEG;
            }
        }

        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) { corriendo = 0; break; }
            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_ESCAPE: corriendo = 0; break;
                    case SDLK_p:
                        pausado = !pausado;
                        ultimo  = SDL_GetTicks();
                        break;
                    case SDLK_0:
                        fase = 0; segundos = TRABAJO_SEG;
                        pausado = 0; ciclos = 0;
                        ultimo = SDL_GetTicks();
                        break;
                    default: break;
                }
            }
        }

        // --- render ---
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);

        // borde exterior
        SDL_SetRenderDrawColor(renderer, 0, 180, 60, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){5, 5, ancho-10, alto-10});

        // barra de titulo
        SDL_SetRenderDrawColor(renderer, 0, 60, 20, 255);
        SDL_RenderFillRect(renderer, &(SDL_Rect){5, 5, ancho-10, 28});
        dibujadoTexto(renderer, fuente,
            "POMODORO   [ESC] volver  [p] pausa  [0] reiniciar", 15, 8);

        int cx = ancho / 2;
        int cy = alto  / 2;

        // fase
        const char *fase_txt = (fase == 0) ? "TRABAJO" : "DESCANSO";
        SDL_Color   c_fase   = (fase == 0) ? c_verde : c_amarillo;
        int fw; TTF_SizeUTF8(fuente, fase_txt, &fw, NULL);
        dibujadoTextoColor(renderer, fuente, fase_txt,
            cx - fw/2 - 10, cy - 90, c_fase);

        // linea decorativa
        SDL_SetRenderDrawColor(renderer, 0, 100, 40, 200);
        SDL_RenderDrawLine(renderer, cx - 120, cy - 60, cx + 120, cy - 60);

        // timer MM:SS
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", segundos/60, segundos%60);
        int tw; TTF_SizeUTF8(fuente, buf, &tw, NULL);
        SDL_Color c_timer = pausado ? c_gris : c_verde;
        dibujadoTextoColor(renderer, fuente, buf,
            cx - tw/2 - 10, cy - 50, c_timer);

        // ciclos completados
        char cbuf[40];
        snprintf(cbuf, sizeof(cbuf), "Ciclos completados: %d", ciclos);
        int cw; TTF_SizeUTF8(fuente, cbuf, &cw, NULL);
        dibujadoTextoColor(renderer, fuente, cbuf,
            cx - cw/2 - 10, cy + 10, c_gris);

        // cartel PAUSADO
        if (pausado) {
            const char *p_txt = "[ PAUSADO ]";
            int pw; TTF_SizeUTF8(fuente, p_txt, &pw, NULL);
            dibujadoTextoColor(renderer, fuente, p_txt,
                cx - pw/2 - 10, cy + 40, c_amarillo);
        }

        // ayuda pie
        const char *help = "[p] pausar   [0] reiniciar   [ESC] volver";
        int hw; TTF_SizeUTF8(fuente, help, &hw, NULL);
        dibujadoTextoColor(renderer, fuente, help,
            cx - hw/2 - 10, alto - 50, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
}

#else
// ── Linux/macOS: pomodoro via bash + PTY (proceso global) ────────────────
int
screenPomodoro(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    // Arrancar el proceso si no esta corriendo todavia
    pom_init();

    // Fuente monoespaciada opcional
    TTF_Font *mono = TTF_OpenFont("assets/fonts/mono.ttf", 16);
    TTF_Font *f    = mono ? mono : fuente;

    int line_h = TTF_FontHeight(f) + 1;
    int char_w = 8;
    { int w, h; if (TTF_SizeUTF8(f, "A", &w, &h) == 0) char_w = w; }

    int margin_x = 20;
    int margin_y = 20;

    SDL_StartTextInput();

    int       corriendo = 1;
    SDL_Event evento;
    SDL_Color verde       = {0, 220, 80,  255};
    SDL_Color verde_fondo = {0,  30, 10,  255};

    while (corriendo) {

        // Leer output pendiente del proceso global
        pom_tick();

        // Eventos
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) { corriendo = 0; break; }

            if (evento.type == SDL_KEYDOWN) {
                switch (evento.key.keysym.sym) {
                    case SDLK_ESCAPE: corriendo = 0; break;
                    case SDLK_RETURN:    pom_send("\n",   1); break;
                    case SDLK_BACKSPACE: pom_send("\x7f", 1); break;
                    default: break;
                }
            }

            // Todos los caracteres (incluido 9, p, 0, etc.) → al script
            if (evento.type == SDL_TEXTINPUT)
                pom_send(evento.text.text, (int)strlen(evento.text.text));
        }

        // --- Render ---
        SDL_SetRenderDrawColor(renderer, verde_fondo.r, verde_fondo.g, verde_fondo.b, 255);
        SDL_RenderClear(renderer);

        // Borde exterior
        SDL_SetRenderDrawColor(renderer, 0, 180, 60, 255);
        SDL_Rect borde = {5, 5, ancho - 10, alto - 10};
        SDL_RenderDrawRect(renderer, &borde);

        // Barra de titulo
        SDL_Rect title_bar = {5, 5, ancho - 10, 28};
        SDL_SetRenderDrawColor(renderer, 0, 60, 20, 255);
        SDL_RenderFillRect(renderer, &title_bar);
        dibujadoTexto(renderer, fuente,
            "POMODORO   [ESC] volver  [p] pausa  [0] detener", 15, 8);

        // Grilla VT
        for (int row = 0; row < POM_ROWS; row++) {
            int y = margin_y + 30 + row * line_h;
            if (y + line_h > alto - 10) break;

            char linea[POM_COLS + 1];
            memcpy(linea, pom_grid[row], POM_COLS);
            linea[POM_COLS] = '\0';

            int len = POM_COLS - 1;
            while (len >= 0 && linea[len] == ' ') len--;
            linea[len + 1] = '\0';
            if (linea[0] == '\0') continue;

            SDL_Surface *sup = TTF_RenderUTF8_Blended(f, linea, verde);
            if (!sup) continue;
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
            SDL_Rect pos = {margin_x, y, sup->w, sup->h};
            SDL_RenderCopy(renderer, tex, NULL, &pos);
            SDL_FreeSurface(sup);
            SDL_DestroyTexture(tex);
        }

        // Cursor parpadeante verde
        if ((SDL_GetTicks() / 500) % 2 == 0) {
            int cur_x = margin_x + pom_col * char_w;
            int cur_y = margin_y + 30 + pom_row * line_h;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 255, 80, 180);
            SDL_Rect cur_rect = {cur_x, cur_y, char_w, line_h};
            SDL_RenderFillRect(renderer, &cur_rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    if (mono) TTF_CloseFont(mono);
    return 0;
    // NO se mata el proceso — sigue corriendo en fondo
}
#endif // _WIN32
