#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef _WIN32
#  include <unistd.h>
#  include <fcntl.h>
#  include <pty.h>
#  include <sys/wait.h>
#endif
#include "ui.h"
#include "pomodoro_bg.h"

// ---- Mini emulador VT100 para bim.sh ----
#define BIM_ROWS 55
#define BIM_COLS 110

static char bim_grid[BIM_ROWS][BIM_COLS];
static int  bim_row = 0, bim_col = 0;

static void bim_clear(void) {
    memset(bim_grid, ' ', sizeof(bim_grid));
    bim_row = 0; bim_col = 0;
}

static void bim_putchar(char c) {
    unsigned char uc = (unsigned char)c;
    if (c == '\r') { bim_col = 0; return; }
    if (c == '\n') {
        bim_row++;
        bim_col = 0;
        if (bim_row >= BIM_ROWS) bim_row = BIM_ROWS - 1;
        return;
    }
    if (c == '\b') { if (bim_col > 0) bim_col--; return; }

    if (uc < 0x20 || uc > 0x7E) return;

    if (bim_row < BIM_ROWS && bim_col < BIM_COLS)
        bim_grid[bim_row][bim_col++] = c;
}

static void bim_process(const char *buf, int n) {
    int i = 0;
    while (i < n) {
        unsigned char c = (unsigned char)buf[i];

        if (c == '\033') {
            i++;
            if (i >= n) break;

            if (buf[i] == '[') {
                i++;
                char params[64] = ""; int pi = 0;
                while (i < n && pi < 63 &&
                       (buf[i] == ';' || buf[i] == '?' ||
                        (buf[i] >= '0' && buf[i] <= '9')))
                    params[pi++] = buf[i++];
                params[pi] = '\0';
                if (i >= n) break;
                char cmd = buf[i++];
                switch (cmd) {
                    case 'H': case 'f': {
                        int r = 0, co = 0;
                        sscanf(params, "%d;%d", &r, &co);
                        bim_row = (r  > 0 ? r  - 1 : 0);
                        bim_col = (co > 0 ? co - 1 : 0);
                        if (bim_row >= BIM_ROWS) bim_row = BIM_ROWS - 1;
                        if (bim_col >= BIM_COLS) bim_col = BIM_COLS - 1;
                        break;
                    }
                    case 'J': memset(bim_grid, ' ', sizeof(bim_grid)); break;
                    case 'K':
                        if (bim_row < BIM_ROWS)
                            memset(&bim_grid[bim_row][bim_col], ' ', BIM_COLS - bim_col);
                        break;
                    case 'A': { int d = params[0] ? atoi(params) : 1; bim_row -= d; if (bim_row < 0) bim_row = 0; break; }
                    case 'B': { int d = params[0] ? atoi(params) : 1; bim_row += d; if (bim_row >= BIM_ROWS) bim_row = BIM_ROWS-1; break; }
                    case 'C': { int d = params[0] ? atoi(params) : 1; bim_col += d; if (bim_col >= BIM_COLS) bim_col = BIM_COLS-1; break; }
                    case 'D': { int d = params[0] ? atoi(params) : 1; bim_col -= d; if (bim_col < 0) bim_col = 0; break; }
                    default: break;
                }

            } else if (buf[i] == ']') {
                i++;
                while (i < n) {
                    if ((unsigned char)buf[i] == 0x07) { i++; break; }
                    if ((unsigned char)buf[i] == '\033' && i+1 < n &&
                        buf[i+1] == '\\') { i += 2; break; }
                    i++;
                }

            } else {
                i++;
            }

        } else if (c == '\r' || c == '\n' || c == '\b') {
            bim_putchar((char)c);
            i++;
        } else {
            bim_putchar((char)c);
            i++;
        }
    }
}

/* ------------------------------------------------------------------
 * run_paed
 * Ejecuta Frankly/paed sobre un .paed y llena out[][256] con la salida.
 * paed usa "source ./flags.sh" así que necesita correr desde Frankly/.
 * ------------------------------------------------------------------ */
#ifndef _WIN32
static int
run_paed(const char *savefile, char out[][256], int max_lines)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "cd Frankly && ./paed '../%s' 2>&1", savefile);

    FILE *fp = popen(cmd, "r");
    if (!fp) {
        strncpy(out[0], "error: no se pudo ejecutar paed", 255);
        return 1;
    }

    int  n = 0;
    char line[512];
    while (n < max_lines && fgets(line, sizeof(line), fp)) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        strncpy(out[n], line, 255);
        out[n][255] = '\0';
        n++;
    }
    pclose(fp);
    return n > 0 ? n : 0;
}
#endif /* _WIN32 */

/* ------------------------------------------------------------------
 * screenFreeEditor
 * Editor libre: bim ocupa ~80% de la pantalla (con barra de título).
 * Debajo, un panel "Output" muestra salida con prefijo ">".
 * ------------------------------------------------------------------ */
#ifdef _WIN32
int
screenFreeEditor(SDL_Renderer *renderer, TTF_Font *fuente,
                 int ancho, int alto, int nivel_num)
{
    (void)nivel_num;
    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)   return 0;
            if (e.type == SDL_KEYDOWN) running = 0;
        }
        SDL_SetRenderDrawColor(renderer, 10, 18, 10, 255);
        SDL_RenderClear(renderer);
        SDL_Color c = {0, 200, 80, 255};
        dibujadoTextoColor(renderer, fuente, "bimEditor - Editor Libre", 20, 20, c);
        SDL_Color cg = {100, 100, 100, 255};
        dibujadoTextoColor(renderer, fuente, "No disponible en Windows", 20, 60, cg);
        dibujadoTextoColor(renderer, fuente, "[cualquier tecla] volver",  20, 90, cg);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
}
#else
int
screenFreeEditor(SDL_Renderer *renderer, TTF_Font *fuente,
                 int ancho, int alto, int nivel_num)
{
    (void)nivel_num;

    /* ── Layout ──────────────────────────────────────────────── */
    int line_h  = TTF_FontHeight(fuente) + 1;
    int font_h  = TTF_FontHeight(fuente);
    const int TITLE_H   = font_h + 4;   /* margen 2px arriba y abajo */
    const int CON_TH    = font_h + 4;
    const int CON_H     = alto / 5;
    const int editor_h  = alto - CON_H;
    int char_w = 9;

    int bim_w = ancho / char_w;
    int bim_h = (editor_h - TITLE_H) / line_h;
    if (bim_w > BIM_COLS) bim_w = BIM_COLS;
    if (bim_h > BIM_ROWS) bim_h = BIM_ROWS;

    bim_clear();

    /* ── Lanzar bim.sh en un PTY ─────────────────────────────── */
    struct winsize ws = {(unsigned short)bim_h, (unsigned short)bim_w, 0, 0};
    int   master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, &ws);
    if (pid < 0) return 0;

    if (pid == 0) {
        chdir("scripts/editorBim");
        setenv("TERM", "xterm", 1);
        execl("/bin/bash", "bash", "bim.sh", "../../saves/libre.paed", NULL);
        _exit(1);
    }

    fcntl(master_fd, F_SETFL, O_NONBLOCK);
    SDL_StartTextInput();

    /* ── Mensajes iniciales de la consola ────────────────────── */
#define FREE_MAX_OUT 8
    char out_buf[FREE_MAX_OUT][256];
    int  n_out = 0;
    snprintf(out_buf[n_out++], 256, "listo — F12 para guardar, F5 para ejecutar");
    snprintf(out_buf[n_out++], 256, "el output de tu codigo aparece aqui abajo");

    /* ── Colores ─────────────────────────────────────────────── */
    SDL_Color c_bim      = {0, 220, 80, 255};
    SDL_Color c_out      = {0, 195, 65, 255};
    SDL_Color c_out_dim  = {0, 110, 38, 255};
    SDL_Color c_title_ed = {0, 235, 85, 255};
    SDL_Color c_title_co = {0, 155, 50, 255};
    SDL_Color c_hint     = {0, 100, 38, 255};

    SDL_Rect editor_area  = {0, 0,        ancho, editor_h};
    SDL_Rect bim_clip     = {0, TITLE_H,  ancho, editor_h - TITLE_H};
    SDL_Rect console_area = {0, editor_h, ancho, CON_H};

    int corriendo = 1;
    SDL_Event evento;
    char io_buf[4096];

    while (corriendo) {

        pom_tick();

        int nr = read(master_fd, io_buf, sizeof(io_buf) - 1);
        if (nr > 0) { io_buf[nr] = '\0'; bim_process(io_buf, nr); }

        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) { corriendo = 0; break; }

            if (evento.type == SDL_KEYDOWN) {
                SDL_Keycode k   = evento.key.keysym.sym;
                SDL_Keymod  mod = SDL_GetModState();

                // F5 = ejecutar paed sobre saves/libre.paed
                if (k == SDLK_F5) {
                    n_out = run_paed("saves/libre.paed", out_buf, FREE_MAX_OUT);
                    break;
                }
                if (k == SDLK_F8)  { screenDoc(renderer, fuente, ancho, alto); break; }
                if (k == SDLK_F12) { write(master_fd, ":w\n", 3); break; }
                if (k == SDLK_F10)  { corriendo = 0; break; }
                if ((mod & KMOD_CTRL) && k == SDLK_p)       { pom_send("p", 1); break; }
                if ((mod & KMOD_CTRL) && k == SDLK_0)       { pom_send("0", 1); break; }

                switch (k) {
                    case SDLK_ESCAPE:    write(master_fd, "\033",    1); break;
                    case SDLK_RETURN:    write(master_fd, "\n",      1); break;
                    case SDLK_BACKSPACE: write(master_fd, "\x7f",    1); break;
                    case SDLK_UP:        write(master_fd, "\033[A",  3); break;
                    case SDLK_DOWN:      write(master_fd, "\033[B",  3); break;
                    case SDLK_RIGHT:     write(master_fd, "\033[C",  3); break;
                    case SDLK_LEFT:      write(master_fd, "\033[D",  3); break;
                    case SDLK_HOME:      write(master_fd, "\033[H",  3); break;
                    case SDLK_END:       write(master_fd, "\033[F",  3); break;
                    case SDLK_DELETE:    write(master_fd, "\033[3~", 4); break;
                    default:
                        if ((mod & KMOD_CTRL) && k >= SDLK_a && k <= SDLK_z) {
                            char ctrl = (char)(k - SDLK_a + 1);
                            write(master_fd, &ctrl, 1);
                        }
                        break;
                }
            }

            if (evento.type == SDL_TEXTINPUT)
                write(master_fd, evento.text.text, strlen(evento.text.text));
        }

        /* ── RENDER ─────────────────────────────────────────── */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 10, 14, 10, 255);
        SDL_RenderFillRect(renderer, &editor_area);

        SDL_RenderSetClipRect(renderer, &bim_clip);
        for (int row = 0; row < bim_h && row < BIM_ROWS; row++) {
            int y = TITLE_H + row * line_h;
            if (y >= editor_h) break;

            char linea[BIM_COLS + 1];
            memcpy(linea, bim_grid[row], BIM_COLS);
            linea[BIM_COLS] = '\0';
            int len = BIM_COLS - 1;
            while (len >= 0 && linea[len] == ' ') len--;
            linea[len + 1] = '\0';
            if (linea[0] == '\0') continue;

            SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente, linea, c_bim);
            if (!sup) continue;
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
            SDL_Rect pos = {4, y, sup->w, sup->h};
            SDL_RenderCopy(renderer, tex, NULL, &pos);
            SDL_FreeSurface(sup);
            SDL_DestroyTexture(tex);
        }

        if ((SDL_GetTicks() / 500) % 2 == 0) {
            int cur_x = 4 + bim_col * char_w;
            int cur_y = TITLE_H + bim_row * line_h;
            if (cur_y >= TITLE_H && cur_y < editor_h) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 255, 80, 175);
                SDL_Rect cur = {cur_x, cur_y, char_w, line_h};
                SDL_RenderFillRect(renderer, &cur);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);

        SDL_SetRenderDrawColor(renderer, 0, 55, 18, 255);
        SDL_Rect title_bar = {0, 0, ancho, TITLE_H};
        SDL_RenderFillRect(renderer, &title_bar);
        SDL_SetRenderDrawColor(renderer, 0, 150, 50, 255);
        SDL_RenderDrawLine(renderer, 0, TITLE_H - 1, ancho, TITLE_H - 1);

        {
            SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, "EDITOR LIBRE", c_title_ed);
            if (s) { SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {10, 2, s->w, s->h}; SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t); }
        }
        {
            const char *h = "saves/libre.paed";
            int hw; TTF_SizeUTF8(fuente, h, &hw, NULL);
            SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, h, c_hint);
            if (s) { SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {ancho - hw - 10, 2, s->w, s->h}; SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t); }
        }

        SDL_SetRenderDrawColor(renderer, 6, 12, 6, 255);
        SDL_RenderFillRect(renderer, &console_area);

        SDL_SetRenderDrawColor(renderer, 0, 170, 55, 255);
        SDL_RenderDrawLine(renderer, 0, editor_h, ancho, editor_h);
        SDL_SetRenderDrawColor(renderer, 0, 70, 22, 255);
        SDL_RenderDrawLine(renderer, 0, editor_h + 1, ancho, editor_h + 1);

        SDL_Rect con_title = {0, editor_h + 2, ancho, CON_TH};
        SDL_SetRenderDrawColor(renderer, 0, 38, 13, 255);
        SDL_RenderFillRect(renderer, &con_title);
        {
            SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, "SALIDA", c_title_co);
            if (s) { SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {10, editor_h + 4, s->w, s->h}; SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t); }
        }
        {
            const char *sc = "[F5] Ejecutar   [F8] Doc   [F12] Guardar   [F10] Salir";
            SDL_Color c_sc = {0, 90, 32, 255};
            int sw; TTF_SizeUTF8(fuente, sc, &sw, NULL);
            SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, sc, c_sc);
            if (s) { SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {ancho - sw - 10, editor_h + 4, s->w, s->h}; SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t); }
        }
        SDL_SetRenderDrawColor(renderer, 0, 55, 18, 160);
        SDL_RenderDrawLine(renderer, 0, editor_h + CON_TH + 2, ancho, editor_h + CON_TH + 2);

        SDL_RenderSetClipRect(renderer, &console_area);
        int con_y = editor_h + CON_TH + 6;

        for (int i = 0; i < n_out; i++) {
            if (con_y + line_h > alto) break;
            char line_str[300];
            snprintf(line_str, sizeof(line_str), "> %s", out_buf[i]);
            SDL_Color c = (i < 2) ? c_out_dim : c_out;
            dibujadoTextoColor(renderer, fuente, line_str, 10, con_y, c);
            con_y += line_h + 2;
        }

        /* sin cursor en el Output — el cursor vive solo en bimEditor */

        SDL_RenderSetClipRect(renderer, NULL);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);

        if (waitpid(pid, NULL, WNOHANG) == pid) corriendo = 0;
    }

    SDL_StopTextInput();
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
    close(master_fd);
    return 0;
}
#endif /* _WIN32 */
