#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "pomodoro_bg.h"

#ifdef _WIN32
// ── Pomodoro nativo SDL2 para Windows (sin bash/forkpty) ─────────────────

/* renderiza texto sin el offset interno de dibujadoTextoColor */
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

/* dibuja el encabezado comun con el titulo POMODORO */
static void
pom_header(SDL_Renderer *r, TTF_Font *ft, TTF_Font *fuente,
           int ancho, const char *subtitulo)
{
    SDL_Color c_verde  = {0, 220, 80, 255};
    SDL_Color c_gris   = {90, 90, 90, 255};
    int cx = ancho / 2;

    /* marco exterior */
    SDL_SetRenderDrawColor(r, 0, 160, 50, 255);
    SDL_RenderDrawRect(r, &(SDL_Rect){4, 4, ancho-8, 56});
    SDL_SetRenderDrawColor(r, 0, 60, 20, 255);
    SDL_RenderFillRect(r, &(SDL_Rect){5, 5, ancho-10, 54});

    /* titulo */
    int tw, th; TTF_SizeUTF8(ft, "POMODORO", &tw, &th);
    pom_txt(r, ft, "POMODORO", cx - tw/2, 8, c_verde);

    /* subtitulo a la derecha */
    if (subtitulo && subtitulo[0]) {
        int sw; TTF_SizeUTF8(fuente, subtitulo, &sw, NULL);
        pom_txt(r, fuente, subtitulo, ancho - sw - 18, 20, c_gris);
    }
}

/* dibuja el timer MM:SS grande centrado */
static void
pom_timer(SDL_Renderer *r, TTF_Font *fr, TTF_Font *fuente,
          int cx, int cy, int seg, int pausado,
          const char *label, SDL_Color c_label)
{
    SDL_Color c_verde  = {0, 220, 80, 255};
    SDL_Color c_gris   = {90, 90, 90, 255};

    /* label de modo */
    int lw; TTF_SizeUTF8(fuente, label, &lw, NULL);
    pom_txt(r, fuente, label, cx - lw/2, cy - 110, c_label);

    /* linea */
    SDL_SetRenderDrawColor(r, 0, 120, 40, 180);
    SDL_RenderDrawLine(r, cx-180, cy-86, cx+180, cy-86);

    /* timer */
    char buf[16];
    snprintf(buf, sizeof(buf), "%02d:%02d", seg/60, seg%60);
    int tw, th; TTF_SizeUTF8(fr, buf, &tw, &th);
    SDL_Color c_t = pausado ? c_gris : c_verde;
    pom_txt(r, fr, buf, cx - tw/2 + 2, cy - 74, (SDL_Color){0,50,15,255}); /* sombra */
    pom_txt(r, fr, buf, cx - tw/2,     cy - 76, c_t);

    /* linea inferior */
    SDL_SetRenderDrawColor(r, 0, 80, 30, 180);
    SDL_RenderDrawLine(r, cx-180, cy + th - 68, cx+180, cy + th - 68);
}

/* ── MODO MENU ── */
static int
pom_menu(SDL_Renderer *renderer, TTF_Font *ft, TTF_Font *fuente, int ancho, int alto)
{
    SDL_Color c_fondo   = {0, 20, 8, 255};
    SDL_Color c_gris    = {90, 90, 90, 255};
    SDL_Color c_blanco  = {200, 220, 200, 255};
    SDL_Color c_sel     = {0, 255, 100, 255};

    const char *opciones[] = {
        "[1]  Pomodoro  (25 + 5 min)",
        "[2]  Cronometro",
        "[3]  Temporizador  (custom)",
        "[4]  Estadisticas",
        "[ESC] Volver al juego",
    };
    int n = 5;
    int sel = 0;
    int lh = TTF_FontHeight(fuente) + 8;
    int cx = ancho / 2;
    int corriendo = 1;
    SDL_Event ev;

    while (corriendo) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) return -1;
            if (ev.type == SDL_KEYDOWN) {
                switch (ev.key.keysym.sym) {
                    case SDLK_ESCAPE: return 0;
                    case SDLK_UP:     sel = (sel - 1 + n) % n; break;
                    case SDLK_DOWN:   sel = (sel + 1) % n; break;
                    case SDLK_RETURN: return sel + 1;
                    case SDLK_1: return 1;
                    case SDLK_2: return 2;
                    case SDLK_3: return 3;
                    case SDLK_4: return 4;
                    default: break;
                }
            }
        }

        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);

        /* marco exterior */
        SDL_SetRenderDrawColor(renderer, 0, 160, 50, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4, 4, ancho-8, alto-8});
        SDL_SetRenderDrawColor(renderer, 0, 80, 30, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){8, 8, ancho-16, alto-16});

        pom_header(renderer, ft, fuente, ancho, "MENU");

        /* caja del menu */
        int menu_w = 400, menu_x = cx - menu_w/2;
        int menu_y = 80;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0, 40, 15, 180);
        SDL_RenderFillRect(renderer, &(SDL_Rect){menu_x, menu_y,
                                                  menu_w, n*lh + 24});
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0, 140, 50, 255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){menu_x, menu_y,
                                                  menu_w, n*lh + 24});

        for (int i = 0; i < n; i++) {
            int oy = menu_y + 12 + i * lh;
            SDL_Color c = (i == sel) ? c_sel : c_blanco;
            if (i == n-1) c = c_gris; /* ESC en gris */
            int tw; TTF_SizeUTF8(fuente, opciones[i], &tw, NULL);
            if (i == sel) {
                SDL_SetRenderDrawColor(renderer, 0, 80, 30, 255);
                SDL_RenderFillRect(renderer, &(SDL_Rect){menu_x+4, oy-2,
                                                          menu_w-8, lh});
            }
            pom_txt(renderer, fuente, opciones[i], cx - tw/2, oy, c);
        }

        /* ayuda */
        const char *h = "[flechas] navegar   [Enter] seleccionar";
        int hw; TTF_SizeUTF8(fuente, h, &hw, NULL);
        pom_txt(renderer, fuente, h, cx - hw/2, alto - 36, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
}

/* ── MODO POMODORO ── */
static int
pom_modo_pomodoro(SDL_Renderer *renderer, TTF_Font *ft, TTF_Font *fr,
                  TTF_Font *fuente, int ancho, int alto, int *p_ciclos)
{
    int TRABAJO  = 25 * 60;
    int DESCANSO =  5 * 60;
    int fase = 0, seg = TRABAJO, pausado = 0;
    Uint32 ultimo = SDL_GetTicks();
    SDL_Color c_fondo = {0,20,8,255};
    SDL_Color c_gris  = {90,90,90,255};
    SDL_Color c_verde = {0,220,80,255};
    SDL_Color c_amar  = {220,200,0,255};
    SDL_Event ev;
    int cx = ancho/2, cy = alto/2;

    while (1) {
        Uint32 ahora = SDL_GetTicks();
        if (!pausado && ahora - ultimo >= 1000) {
            ultimo += 1000; seg--;
            if (seg < 0) {
                fase = 1 - fase;
                if (fase == 0) (*p_ciclos)++;
                seg = (fase == 0) ? TRABAJO : DESCANSO;
            }
        }
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    return -1;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) return 0;
                if (ev.key.keysym.sym == SDLK_p) {
                    pausado = !pausado; ultimo = SDL_GetTicks();
                }
                if (ev.key.keysym.sym == SDLK_0) {
                    fase=0; seg=TRABAJO; pausado=0; *p_ciclos=0;
                    ultimo = SDL_GetTicks();
                }
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0,160,50,255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4,4,ancho-8,alto-8});

        pom_header(renderer, ft, fuente, ancho,
                   fase==0 ? "TRABAJO" : "DESCANSO");
        pom_timer(renderer, fr, fuente, cx, cy, seg, pausado,
                  fase==0 ? "[ TRABAJO ]" : "[ DESCANSO ]",
                  fase==0 ? c_verde : c_amar);

        /* ciclos */
        char cbuf[40];
        snprintf(cbuf, sizeof(cbuf), "Ciclos: %d", *p_ciclos);
        int cw; TTF_SizeUTF8(fuente, cbuf, &cw, NULL);
        pom_txt(renderer, fuente, cbuf, cx - cw/2, cy + 40, c_gris);

        if (pausado && (SDL_GetTicks()/500)%2==0) {
            const char *pt = "** PAUSADO **";
            int pw; TTF_SizeUTF8(fuente, pt, &pw, NULL);
            pom_txt(renderer, fuente, pt, cx-pw/2, cy+68, c_amar);
        }

        const char *help = "[p] pausar   [0] reiniciar   [ESC] menu";
        int hw; TTF_SizeUTF8(fuente, help, &hw, NULL);
        pom_txt(renderer, fuente, help, cx-hw/2, alto-36, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

/* ── MODO CRONOMETRO ── */
static int
pom_modo_cronometro(SDL_Renderer *renderer, TTF_Font *ft, TTF_Font *fr,
                    TTF_Font *fuente, int ancho, int alto)
{
    Uint32 inicio = SDL_GetTicks(), pausa_acum = 0, pausa_inicio = 0;
    int pausado = 0;
    Uint32 elapsed = 0;
    SDL_Color c_fondo = {0,20,8,255};
    SDL_Color c_gris  = {90,90,90,255};
    SDL_Color c_cyan  = {0,200,220,255};
    SDL_Event ev;
    int cx = ancho/2, cy = alto/2;

    while (1) {
        Uint32 ahora = SDL_GetTicks();
        if (!pausado)
            elapsed = (ahora - inicio - pausa_acum) / 1000;

        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    return -1;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) return 0;
                if (ev.key.keysym.sym == SDLK_p) {
                    if (!pausado) { pausado=1; pausa_inicio=ahora; }
                    else { pausa_acum += ahora - pausa_inicio; pausado=0; }
                }
                if (ev.key.keysym.sym == SDLK_0) {
                    inicio=SDL_GetTicks(); pausa_acum=0; elapsed=0; pausado=0;
                }
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0,160,50,255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4,4,ancho-8,alto-8});

        pom_header(renderer, ft, fuente, ancho, "CRONOMETRO");
        pom_timer(renderer, fr, fuente, cx, cy,
                  (int)elapsed, pausado, "[ CRONOMETRO ]", c_cyan);

        if (pausado && (SDL_GetTicks()/500)%2==0) {
            const char *pt = "** PAUSADO **";
            int pw; TTF_SizeUTF8(fuente, pt, &pw, NULL);
            pom_txt(renderer, fuente, pt, cx-pw/2, cy+68,
                    (SDL_Color){220,200,0,255});
        }

        const char *help = "[p] pausar   [0] reiniciar   [ESC] menu";
        int hw; TTF_SizeUTF8(fuente, help, &hw, NULL);
        pom_txt(renderer, fuente, help, cx-hw/2, alto-36, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

/* ── MODO TEMPORIZADOR CUSTOM ── */
static int
pom_modo_timer(SDL_Renderer *renderer, TTF_Font *ft, TTF_Font *fr,
               TTF_Font *fuente, int ancho, int alto)
{
    /* pedir minutos con el teclado */
    char input[8] = ""; int input_len = 0;
    int configurando = 1;
    SDL_Color c_fondo = {0,20,8,255};
    SDL_Color c_verde = {0,220,80,255};
    SDL_Color c_gris  = {90,90,90,255};
    SDL_Color c_amar  = {220,200,0,255};
    SDL_Event ev;
    int cx = ancho/2, cy = alto/2;

    SDL_StartTextInput();

    while (configurando) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    { SDL_StopTextInput(); return -1; }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    SDL_StopTextInput(); return 0;
                }
                if (ev.key.keysym.sym == SDLK_BACKSPACE && input_len > 0)
                    input[--input_len] = '\0';
                if (ev.key.keysym.sym == SDLK_RETURN && input_len > 0)
                    configurando = 0;
            }
            if (ev.type == SDL_TEXTINPUT && input_len < 3 &&
                ev.text.text[0] >= '0' && ev.text.text[0] <= '9') {
                input[input_len++] = ev.text.text[0];
                input[input_len]   = '\0';
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0,160,50,255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4,4,ancho-8,alto-8});
        pom_header(renderer, ft, fuente, ancho, "TEMPORIZADOR");

        const char *preg = "Cuantos minutos?";
        int pw; TTF_SizeUTF8(fuente, preg, &pw, NULL);
        pom_txt(renderer, fuente, preg, cx-pw/2, cy-60, c_verde);

        char disp[16]; snprintf(disp, sizeof(disp), "%s_", input);
        int dw; TTF_SizeUTF8(fr, disp, &dw, NULL);
        pom_txt(renderer, fr, disp, cx-dw/2, cy-20, c_amar);

        const char *h = "[numeros] ingresar   [Enter] confirmar   [ESC] volver";
        int hw; TTF_SizeUTF8(fuente, h, &hw, NULL);
        pom_txt(renderer, fuente, h, cx-hw/2, alto-36, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    SDL_StopTextInput();

    int total = atoi(input) * 60;
    if (total <= 0) total = 60;
    int seg = total, pausado = 0;
    Uint32 ultimo = SDL_GetTicks();

    while (1) {
        Uint32 ahora = SDL_GetTicks();
        if (!pausado && ahora - ultimo >= 1000) {
            ultimo += 1000;
            if (seg > 0) seg--;
        }
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    return -1;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) return 0;
                if (ev.key.keysym.sym == SDLK_p) {
                    pausado = !pausado; ultimo = SDL_GetTicks();
                }
                if (ev.key.keysym.sym == SDLK_0) {
                    seg=total; pausado=0; ultimo=SDL_GetTicks();
                }
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0,160,50,255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4,4,ancho-8,alto-8});
        pom_header(renderer, ft, fuente, ancho, "TEMPORIZADOR");

        char lbl[32]; snprintf(lbl, sizeof(lbl), "[ %d min ]", atoi(input));
        pom_timer(renderer, fr, fuente, cx, cy, seg, pausado, lbl, c_amar);

        if (seg == 0) {
            const char *fin = "** TIEMPO COMPLETADO **";
            int fw; TTF_SizeUTF8(fuente, fin, &fw, NULL);
            pom_txt(renderer, fuente, fin, cx-fw/2, cy+60, c_verde);
        }
        if (pausado && (SDL_GetTicks()/500)%2==0) {
            const char *pt = "** PAUSADO **";
            int pw2; TTF_SizeUTF8(fuente, pt, &pw2, NULL);
            pom_txt(renderer, fuente, pt, cx-pw2/2, cy+60, c_amar);
        }

        const char *help = "[p] pausar   [0] reiniciar   [ESC] menu";
        int hw; TTF_SizeUTF8(fuente, help, &hw, NULL);
        pom_txt(renderer, fuente, help, cx-hw/2, alto-36, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

/* ── MODO ESTADISTICAS ── */
static void
pom_modo_stats(SDL_Renderer *renderer, TTF_Font *ft, TTF_Font *fuente,
               int ancho, int alto, int ciclos)
{
    SDL_Color c_fondo = {0,20,8,255};
    SDL_Color c_verde = {0,220,80,255};
    SDL_Color c_gris  = {90,90,90,255};
    SDL_Color c_amar  = {220,200,0,255};
    SDL_Event ev;
    int cx = ancho/2, cy = alto/2;
    int lh = TTF_FontHeight(fuente) + 10;

    while (1) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT)    return;
            if (ev.type == SDL_KEYDOWN) return;
        }
        SDL_RenderSetClipRect(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, c_fondo.r, c_fondo.g, c_fondo.b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0,160,50,255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){4,4,ancho-8,alto-8});
        pom_header(renderer, ft, fuente, ancho, "ESTADISTICAS");

        int box_w = 360, box_x = cx - box_w/2, box_y = cy - 80;
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 0,40,15,180);
        SDL_RenderFillRect(renderer, &(SDL_Rect){box_x, box_y, box_w, 160});
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer, 0,140,50,255);
        SDL_RenderDrawRect(renderer, &(SDL_Rect){box_x, box_y, box_w, 160});

        char buf[64];
        snprintf(buf, sizeof(buf), "Pomodoros completados: %d", ciclos);
        int bw; TTF_SizeUTF8(fuente, buf, &bw, NULL);
        pom_txt(renderer, fuente, buf, cx-bw/2, box_y+20, c_verde);

        snprintf(buf, sizeof(buf), "Tiempo de trabajo: %d min", ciclos*25);
        TTF_SizeUTF8(fuente, buf, &bw, NULL);
        pom_txt(renderer, fuente, buf, cx-bw/2, box_y+20+lh, c_amar);

        const char *h2 = "Presiona cualquier tecla para volver";
        int h2w; TTF_SizeUTF8(fuente, h2, &h2w, NULL);
        pom_txt(renderer, fuente, h2, cx-h2w/2, alto-36, c_gris);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
}

/* ── ENTRY POINT ── */
int
screenPomodoro(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    TTF_Font *f_titulo = TTF_OpenFont("assets/fonts/main.ttf", 28);
    TTF_Font *f_timer  = TTF_OpenFont("assets/fonts/main.ttf", 64);
    TTF_Font *ft = f_titulo ? f_titulo : fuente;
    TTF_Font *fr = f_timer  ? f_timer  : fuente;

    int ciclos = 0;

    while (1) {
        int opcion = pom_menu(renderer, ft, fuente, ancho, alto);
        if (opcion <= 0) break;  /* 0=ESC, -1=QUIT */
        switch (opcion) {
            case 1:
                if (pom_modo_pomodoro(renderer, ft, fr, fuente,
                                      ancho, alto, &ciclos) < 0)
                    goto fin;
                break;
            case 2:
                if (pom_modo_cronometro(renderer, ft, fr, fuente,
                                        ancho, alto) < 0)
                    goto fin;
                break;
            case 3:
                if (pom_modo_timer(renderer, ft, fr, fuente,
                                   ancho, alto) < 0)
                    goto fin;
                break;
            case 4:
                pom_modo_stats(renderer, ft, fuente, ancho, alto, ciclos);
                break;
            case 5: goto fin;
        }
    }

fin:
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
