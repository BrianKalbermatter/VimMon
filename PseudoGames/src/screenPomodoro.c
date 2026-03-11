#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "pomodoro_bg.h"

#ifdef _WIN32
// ── Pomodoro nativo SDL2 para Windows (sin bash/forkpty) ─────────────────

/* renderiza texto directamente sin el offset +10/+12 de dibujadoTextoColor */
static void
pom_txt(SDL_Renderer *r, TTF_Font *f, const char *s, int x, int y, SDL_Color c)
{
    SDL_Surface *sup = TTF_RenderUTF8_Blended(f, s, c);
    if (!sup) return;
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, sup);
    SDL_Rect pos = {x, y, sup->w, sup->h};
    SDL_RenderCopy(r, tex, NULL, &pos);
    SDL_FreeSurface(sup);
    SDL_DestroyTexture(tex);
}

int
screenPomodoro(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    // fuentes grandes para titulo y timer
    TTF_Font *f_titulo = TTF_OpenFont("assets/fonts/main.ttf", 34);
    TTF_Font *f_timer  = TTF_OpenFont("assets/fonts/main.ttf", 72);
    TTF_Font *ft = f_titulo ? f_titulo : fuente;
    TTF_Font *fr = f_timer  ? f_timer  : fuente;

    int TRABAJO_SEG  = 25 * 60;
    int DESCANSO_SEG =  5 * 60;

    int    fase     = 0;
    int    segundos = TRABAJO_SEG;
    int    pausado  = 0;
    int    ciclos   = 0;
    Uint32 ultimo   = SDL_GetTicks();

    SDL_Color c_verde    = {  0, 220,  80, 255};
    SDL_Color c_fondo    = {  0,  20,   8, 255};
    SDL_Color c_amarillo = {220, 200,   0, 255};
    SDL_Color c_gris     = { 90,  90,  90, 255};
    SDL_Color c_borde    = {  0, 160,  50, 255};

    int corriendo = 1;
    SDL_Event evento;

    while (corriendo) {

        // tick cada segundo
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
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);

        int cx = ancho / 2;
        int cy = alto  / 2;

        // marco exterior doble
        SDL_SetRenderDrawColor(renderer, c_borde.r, c_borde.g, c_borde.b, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4,  4,  ancho-8,  alto-8});
        SDL_SetRenderDrawColor(renderer, 0, 80, 30, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){8,  8,  ancho-16, alto-16});

        // titulo "POMODORO" grande centrado arriba
        {
            const char *titulo = "POMODORO";
            int tw, th;
            TTF_SizeUTF8(ft, titulo, &tw, &th);
            // fondo del titulo
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 60, 20, 200);
            SDL_RenderFillRect(renderer, &(SDL_Rect){cx - tw/2 - 20, 20, tw + 40, th + 16});
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 180, 60, 255);
            SDL_RenderDrawRect(renderer, &(SDL_Rect){cx - tw/2 - 20, 20, tw + 40, th + 16});
            // texto
            pom_txt(renderer, ft, titulo, cx - tw/2, 28, c_verde);
        }

        // fase: TRABAJO / DESCANSO
        {
            const char *fase_txt = (fase == 0) ? "[ TRABAJO ]" : "[ DESCANSO ]";
            SDL_Color   c_fase   = (fase == 0) ? c_verde : c_amarillo;
            int tw, th; TTF_SizeUTF8(fuente, fase_txt, &tw, &th);
            pom_txt(renderer, fuente, fase_txt, cx - tw/2, cy - 110, c_fase);
        }

        // separador
        SDL_SetRenderDrawColor(renderer, 0, 120, 40, 180);
        SDL_RenderDrawLine(renderer, cx - 160, cy - 88, cx + 160, cy - 88);

        // timer MM:SS grande
        {
            char buf[16];
            snprintf(buf, sizeof(buf), "%02d:%02d", segundos/60, segundos%60);
            int tw, th; TTF_SizeUTF8(fr, buf, &tw, &th);
            SDL_Color c_t = pausado ? c_gris : c_verde;
            // sombra
            pom_txt(renderer, fr, buf, cx - tw/2 + 2, cy - 72, (SDL_Color){0,60,20,255});
            pom_txt(renderer, fr, buf, cx - tw/2,     cy - 74, c_t);
        }

        // separador inferior
        SDL_SetRenderDrawColor(renderer, 0, 80, 30, 180);
        SDL_RenderDrawLine(renderer, cx - 160, cy + 14, cx + 160, cy + 14);

        // ciclos
        {
            char cbuf[40];
            snprintf(cbuf, sizeof(cbuf), "Ciclos completados: %d", ciclos);
            int tw, th; TTF_SizeUTF8(fuente, cbuf, &tw, &th);
            pom_txt(renderer, fuente, cbuf, cx - tw/2, cy + 24, c_gris);
        }

        // cartel PAUSADO (parpadeante)
        if (pausado && (SDL_GetTicks() / 500) % 2 == 0) {
            const char *p_txt = "** PAUSADO **";
            int tw, th; TTF_SizeUTF8(fuente, p_txt, &tw, &th);
            pom_txt(renderer, fuente, p_txt, cx - tw/2, cy + 54, c_amarillo);
        }

        // ayuda pie
        {
            const char *help = "[p] pausar   [0] reiniciar   [ESC] volver";
            int tw, th; TTF_SizeUTF8(fuente, help, &tw, &th);
            pom_txt(renderer, fuente, help, cx - tw/2, alto - 38, c_gris);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    if (f_titulo) TTF_CloseFont(f_titulo);
    if (f_timer)  TTF_CloseFont(f_timer);
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
