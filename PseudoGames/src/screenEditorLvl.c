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
#include "niveles.h"
#include "progreso.h"
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

    // Solo caracteres ASCII imprimibles (0x20-0x7E)
    // Los bytes de caracteres UTF-8 multibyte (0x80-0xFF) y los bytes
    // de control los ignoramos — evita que el arte braille del splash
    // y otros simbolos Unicode aparezcan como basura en la grilla
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
                // CSI: \033[ params cmd
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
                // OSC: \033] ... \007 (BEL) o \033] ... \033\ (ST)
                // Saltar todo hasta BEL o hasta \033\ para ignorar
                // titulos de ventana, paletas de color, etc.
                i++;
                while (i < n) {
                    if ((unsigned char)buf[i] == 0x07) { i++; break; } // BEL
                    if ((unsigned char)buf[i] == '\033' && i+1 < n &&
                        buf[i+1] == '\\') { i += 2; break; }           // ST
                    i++;
                }

            } else {
                // Otras secuencias de escape de 2 bytes (\033X) → ignorar
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
 * Ejecuta Frankly/paed sobre un .paed (ruta relativa a la raíz del
 * proyecto) y llena out[][256] con las líneas de stdout+stderr.
 * paed necesita correr desde Frankly/ porque usa "source ./flags.sh".
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

#ifdef _WIN32
int
screenLvLEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num){
    // El editor de bim usa forkpty() que no existe en Windows
    Nivel *nv = obtener_nivel(nivel_num);
    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) return 0;
            if (e.type == SDL_KEYDOWN) running = 0;
        }
        SDL_SetRenderDrawColor(renderer, 10, 18, 10, 255);
        SDL_RenderClear(renderer);
        SDL_Color c = {0, 200, 80, 255};
        dibujadoTextoColor(renderer, fuente,
            nv ? nv->titulo : "Nivel", 20, 20, c);
        SDL_Color cg = {120, 120, 120, 255};
        dibujadoTextoColor(renderer, fuente,
            "Editor no disponible en Windows", 20, 60, cg);
        dibujadoTextoColor(renderer, fuente,
            "[cualquier tecla] volver", 20, 90, cg);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return 0;
}
#else
int
screenLvLEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num){
    
    // forkpty() hace 3 cosas en una: 
    // 1. Crea un PTY - Una terminal falsa, como un tubo bidireccional con superpoderes de terminal.
    // 2. Hace fork() - divide el proceso en dos:
    // mi programa (padre)
    // └──proceso hijo(nuevo)
    // 3. Conecta el PTY al hijo - el hijo tiene stdin/stdout conectados al PTY. Todo lo que el hijo escribe, el padre lo puede leer por fd_pty

//    int fd_pty; // Crea una variable para el proceso de hijo bash deja de ser mi programa y pasa a ser bash corriendo bim.sh
                // file descriptor del PTY - por aca leo. escribo lo que bash produce
//    struct winsize ws = {24, 80, 0, 0}; // 24 filas, 80 columnas
                                        
    // pid_t pid es: Sirve para guardar ID de un proceso, porque cada proceso que corre en linux tiene un numero unico El pid Process ID. Tambien conocido como el DNI del proceso... basicamente es un PID int(1111)
    // forkpty()
    // en el struct creo una ventana de tamano de tanto por tanto...
//    pid_t pid = forkpty(&fd_pty, NULL, NULL, &ws);

//  if (pid == 0) {
      // proceso hijo - acá corre bim.sh
//      chdir("scripts/editorBim");
      //chdir: Cambia el directorio de trabajo del proceso hijo como hacer "cd script/editorBim" en la terminal.
//      execlp("bash", "bash", "bim.sh", "archivoPrueba.txt", NULL);
      //execlp: reemplaza el proceso hijo con bash, deja de ser mi programa y pasa a ser bash corriendo bim.sh
      //el NULL al final marca el fin de los argumentos
//      exit(1);
//  }
  // leer output del PTY y printearlo en consola (para verificar)
  //===========================
  // char buf [4096]:
  //    Es un array de caracteres - 4096, cada una guarda un caracter. 
  //    buf[0] = '\e'
  //    buf[1] = '['
  //    buf[2] = '3'
  //    buf[3] = '8'
  //    buf[4] = ';'
  //    ...
  //    buf[n] = '\0'  ← fin del string
  //    Es lo mismo que &buf[0].
  //    Y un puntero char *p = buf apunta al primer carácter del array. Es lo mismo que
  //    &buf[0].
  //  
  //    p → buf[0] → '\e'
  //    p++ → buf[1] → '['
  //    p++ → buf[2] → '3'
  //    
  //    Y porque 4096 y no 1111 por ejemplo?
  //    Es una convención — 4096 es una potencia de 2 (2¹²).
  //    2¹  = 2
  //    2²  = 4
  //    2³  = 8
  //    2⁴  = 16
  //    2⁸  = 256
  //    2¹²  = 4096
  //    2¹⁶ = 65536

  //    Los sistemas operativos y hardware trabajan internamente con potencias de 2.
  //    Usar 4096 es más eficiente que 1111 porque se alinea con cómo la memoria está organizada.

  // Para tu caso — 4096 caracteres es más que suficiente para guardar un frame completo de bim.sh.
  //
  //===========================
  // que hace el fflush?
  // ->
  
  //char buf[4096];
  //int n = read(fd_pty, buf, sizeof(buf));
  //buf[n] = '\0';
  //printf("%s", buf);
  //fflush(stdout);
  // hacer el fd no bloqueante - sin esto el while se congela esperando a bim.sh
/*  fcntl(fd_pty, F_SETFL, O_NONBLOCK);
    while(corriendo) {
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) corriendo = 0;
            if (evento.type == SDL_KEYDOWN)
                if (evento.key.keysym.sym == SDLK_ESCAPE) corriendo = 0;
        }
         SDL_SetRenderDrawColor(renderer, 0, 0, 0 , 255); // Dibuja un color
         SDL_RenderClear(renderer);
         
          // Barra de titulo - arriba de todo
  SDL_Rect barra_titulo = {0, 0, ancho, 30};
  SDL_SetRenderDrawColor(renderer, 0, 78, 152, 255);
  SDL_RenderFillRect(renderer, &barra_titulo);
  // Texto en la barra de titulo
  SDL_Color blanco = {255, 255, 255, 255};
  SDL_Surface *sup = TTF_RenderText_Solid(fuente, "PseudoGames", blanco);
  SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
  SDL_Rect pos_titulo = {10, 5, 150, 20};
  SDL_RenderCopy(renderer, tex, NULL, &pos_titulo);
  SDL_FreeSurface(sup);
  SDL_DestroyTexture(tex);

         // Panel izquierdo - consigna
         SDL_Rect panel_izq = {0, 30, ancho/2, alto-30};
  SDL_SetRenderDrawColor(renderer, 236, 233, 216, 255);
  SDL_RenderFillRect(renderer, &panel_izq);

  // Panel derecho - editor
  SDL_Rect panel_der = {ancho/2, 30, ancho/2, alto-30};
  SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
  SDL_RenderFillRect(renderer, &panel_der);

  // Línea divisoria
  SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
  SDL_RenderDrawLine(renderer, ancho/2, 0, ancho/2, alto);

  // leer output de bim.sh
  char buf[4096];
  int n = read(fd_pty, buf, sizeof(buf) - 1);
  if (n > 0) {
      buf[n] = '\0';
      char *p = buf;
      while (*p != '\0') {
          if (*p == 27) {
              // es un codigo ANSI - por ahora lo saltamos
              while (*p != 'm' && *p != '\0') p++;
          } else {
              // es texto normal - por ahora lo printeamos
              printf("%c", *p);
          }
          p++;
      }
      fflush(stdout);
  }
}
*/
    // obtener_nivel(nivel_num) da el Nivel * con el titulo y enunciado
    Nivel *n = obtener_nivel(nivel_num);
    if (!n) return 0;

    bim_clear();

    // --- Lanzar bim.sh en el panel derecho via forkpty ---
    // Calcular dimensiones del panel derecho en caracteres
    int line_h      = TTF_FontHeight(fuente) + 1;
    int char_w      = 9;
    int lvl_title_h = 28;          /* barra de título del editor derecho */
    int lvl_con_h   = alto / 5;    /* panel Output abajo (~20%)          */
    int lvl_edit_h  = alto - lvl_con_h; /* zona bim + título             */
    int bim_w   = (ancho / 2) / char_w;
    int bim_h   = (lvl_edit_h - lvl_title_h) / line_h;
    if (bim_w > BIM_COLS) bim_w = BIM_COLS;
    if (bim_h > BIM_ROWS) bim_h = BIM_ROWS;

    struct winsize ws = {(unsigned short)bim_h, (unsigned short)bim_w, 0, 0};
    int   master_fd;
    pid_t pid = forkpty(&master_fd, NULL, NULL, &ws);

    if (pid < 0) return 0;  // error en fork

    if (pid == 0) {
        // HIJO: ir al directorio de bim.sh (fuente otros scripts con rutas relativas)
        chdir("scripts/editorBim");
        setenv("TERM", "xterm", 1);
        // archivo de trabajo para este nivel
        char savefile[128];
        snprintf(savefile, sizeof(savefile), "../../saves/nivel_%d.paed", nivel_num);
        execl("/bin/bash", "bash", "bim.sh", savefile, NULL);
        _exit(1);
    }

    // PADRE: lectura no bloqueante
    fcntl(master_fd, F_SETFL, O_NONBLOCK);

    SDL_StartTextInput();

    int corriendo  = 1;
    SDL_Event evento;
    char buf[4096];

    SDL_Color c_verde  = {180, 255, 180, 255};   // texto del editor
    int panel_x = ancho / 2;
    int panel_w = ancho / 2;

    /* buffer dinámico del panel Output */
#define LVL_MAX_OUT 14
    char lvl_out[LVL_MAX_OUT][256];
    int  n_lvl_out = 2;
    strncpy(lvl_out[0], "listo — guardá con :w y ejecutá con F5", 255);
    strncpy(lvl_out[1], "F5 ejecutar   F6 marcar completado   F10 salir", 255);

    while (corriendo) {

        pom_tick();  // mantener vivo el proceso de pomodoro en fondo

        // Leer output de bim.sh
        int nr = read(master_fd, buf, sizeof(buf) - 1);
        if (nr > 0) { buf[nr] = '\0'; bim_process(buf, nr); }

        // Eventos SDL2
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) { corriendo = 0; break; }

            if (evento.type == SDL_KEYDOWN) {
                SDL_Keycode k   = evento.key.keysym.sym;
                SDL_Keymod  mod = SDL_GetModState();

                // F5 = ejecutar paed sobre el archivo del nivel
                if (k == SDLK_F5) {
                    char savepath[128];
                    snprintf(savepath, sizeof(savepath),
                             "saves/nivel_%d.paed", nivel_num);
                    n_lvl_out = run_paed(savepath, lvl_out, LVL_MAX_OUT);
                    break;
                }
                // F6 = marcar nivel como completado y salir
                if (k == SDLK_F6) { marcar_completado(nivel_num); corriendo = 0; break; }
                // F10 = salir sin completar
                if (k == SDLK_F10) { corriendo = 0; break; }
                // Ctrl+P = pausar pomodoro global | Ctrl+0 = detener
                if ((mod & KMOD_CTRL) && k == SDLK_p) { pom_send("p", 1); break; }
                if ((mod & KMOD_CTRL) && k == SDLK_0) { pom_send("0", 1); break; }

                // Teclas especiales → secuencias de escape de terminal
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
                        // Ctrl + letra
                        if (mod & KMOD_CTRL) {
                            if (k >= SDLK_a && k <= SDLK_z) {
                                char ctrl = (char)(k - SDLK_a + 1); // Ctrl+A=1, Ctrl+S=19
                                write(master_fd, &ctrl, 1);
                            }
                        }
                        break;
                }
            }

            // Texto normal (letras, numeros, simbolos)
            if (evento.type == SDL_TEXTINPUT)
                write(master_fd, evento.text.text, strlen(evento.text.text));
        }

        // ---- Render ----
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Panel izquierdo - consigna
        SDL_Rect panel_izq = {0, 0, panel_x, alto};
        SDL_SetRenderDrawColor(renderer, 10, 18, 10, 255);
        SDL_RenderFillRect(renderer, &panel_izq);

        /* ── Panel derecho: fondo base ── */
        SDL_Rect panel_der = {panel_x, 0, panel_w, alto};
        SDL_SetRenderDrawColor(renderer, 10, 14, 10, 255);
        SDL_RenderFillRect(renderer, &panel_der);

        /* línea divisoria izq / der */
        SDL_SetRenderDrawColor(renderer, 0, 100, 40, 255);
        SDL_RenderDrawLine(renderer, panel_x, 0, panel_x, alto);

        /* ── Consigna (panel izquierdo) ── */
        SDL_Color verde_titulo = {0, 230, 80, 255};
        dibujadoTextoColor(renderer, fuente, n->titulo, 10, 8, verde_titulo);
        SDL_SetRenderDrawColor(renderer, 0, 130, 50, 255);
        SDL_RenderDrawLine(renderer, 10, 42, panel_x - 10, 42);
        SDL_Color verde_texto = {0, 190, 70, 255};
        dibujadoTextoMultilineaColor(renderer, fuente, n->enunciado,
                                     10, 44, panel_x - 30, verde_texto);
        SDL_Color c_help = {50, 90, 60, 255};
        dibujadoTextoColor(renderer, fuente,
            "[F5] ejecutar   [F6] completar   [F10] volver", 10, alto - 30, c_help);

        /* ── Grilla bim (clip entre título y consola) ── */
        SDL_Rect bim_clip_r = {panel_x, lvl_title_h,
                               panel_w, lvl_edit_h - lvl_title_h};
        SDL_RenderSetClipRect(renderer, &bim_clip_r);

        for (int row = 0; row < bim_h && row < BIM_ROWS; row++) {
            int y = lvl_title_h + row * line_h;
            if (y >= lvl_edit_h) break;

            char linea[BIM_COLS + 1];
            memcpy(linea, bim_grid[row], BIM_COLS);
            linea[BIM_COLS] = '\0';
            int len = BIM_COLS - 1;
            while (len >= 0 && linea[len] == ' ') len--;
            linea[len + 1] = '\0';
            if (linea[0] == '\0') continue;

            SDL_Surface *sup = TTF_RenderUTF8_Blended(fuente, linea, c_verde);
            if (!sup) continue;
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, sup);
            SDL_Rect pos = {panel_x + 4, y, sup->w, sup->h};
            SDL_RenderCopy(renderer, tex, NULL, &pos);
            SDL_FreeSurface(sup);
            SDL_DestroyTexture(tex);
        }

        /* cursor parpadeante */
        if ((SDL_GetTicks() / 500) % 2 == 0) {
            int cur_x = panel_x + 4 + bim_col * char_w;
            int cur_y = lvl_title_h + bim_row * line_h;
            if (cur_y >= lvl_title_h && cur_y < lvl_edit_h) {
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 0, 255, 80, 180);
                SDL_Rect cur_rect = {cur_x, cur_y, char_w, line_h};
                SDL_RenderFillRect(renderer, &cur_rect);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);

        /* ── Barra de título del editor (encima del bim) ── */
        SDL_Rect lvl_title_bar = {panel_x, 0, panel_w, lvl_title_h};
        SDL_SetRenderDrawColor(renderer, 0, 55, 18, 255);
        SDL_RenderFillRect(renderer, &lvl_title_bar);
        SDL_SetRenderDrawColor(renderer, 0, 150, 50, 255);
        SDL_RenderDrawLine(renderer, panel_x, lvl_title_h - 1,
                           panel_x + panel_w, lvl_title_h - 1);
        {
            SDL_Color c_ted = {0, 235, 85, 255};
            dibujadoTextoColor(renderer, fuente, "bimEditor", panel_x + 12, 6, c_ted);
            char savename[64];
            snprintf(savename, sizeof(savename), "nivel_%d.paed", nivel_num);
            int hw; TTF_SizeUTF8(fuente, savename, &hw, NULL);
            SDL_Color c_hint = {0, 100, 38, 255};
            dibujadoTextoColor(renderer, fuente, savename,
                               panel_x + panel_w - hw - 12, 6, c_hint);
        }

        /* ── Panel Output (bottom del panel derecho) ── */
        {
            const int CON_TH = 24;
            SDL_Rect con_area = {panel_x, lvl_edit_h, panel_w, lvl_con_h};
            SDL_SetRenderDrawColor(renderer, 6, 12, 6, 255);
            SDL_RenderFillRect(renderer, &con_area);

            /* separador */
            SDL_SetRenderDrawColor(renderer, 0, 170, 55, 255);
            SDL_RenderDrawLine(renderer, panel_x, lvl_edit_h,
                               panel_x + panel_w, lvl_edit_h);
            SDL_SetRenderDrawColor(renderer, 0, 70, 22, 255);
            SDL_RenderDrawLine(renderer, panel_x, lvl_edit_h + 1,
                               panel_x + panel_w, lvl_edit_h + 1);

            /* barra de título Output */
            SDL_Rect con_title = {panel_x, lvl_edit_h + 2, panel_w, CON_TH};
            SDL_SetRenderDrawColor(renderer, 0, 38, 13, 255);
            SDL_RenderFillRect(renderer, &con_title);
            SDL_Color c_ct = {0, 155, 50, 255};
            dibujadoTextoColor(renderer, fuente, "[ Output ]",
                               panel_x + 10, lvl_edit_h + 4, c_ct);
            SDL_SetRenderDrawColor(renderer, 0, 55, 18, 160);
            SDL_RenderDrawLine(renderer, panel_x, lvl_edit_h + CON_TH + 2,
                               panel_x + panel_w, lvl_edit_h + CON_TH + 2);

            /* líneas de output con clip */
            SDL_RenderSetClipRect(renderer, &con_area);
            int con_y = lvl_edit_h + CON_TH + 6;
            for (int mi = 0; mi < n_lvl_out; mi++) {
                if (con_y + line_h > alto) break;
                char mstr[300];
                snprintf(mstr, sizeof(mstr), "> %s", lvl_out[mi]);
                /* las 2 primeras líneas son info (más tenues),
                   el resto es output real de paed (más brillante) */
                SDL_Color c_msg = (mi < 2)
                    ? (SDL_Color){0, 110, 38, 255}
                    : (SDL_Color){0, 210, 75, 255};
                dibujadoTextoColor(renderer, fuente, mstr,
                                   panel_x + 10, con_y, c_msg);
                con_y += line_h + 2;
            }
            /* sin cursor en el Output — el cursor vive solo en bimEditor */
            SDL_RenderSetClipRect(renderer, NULL);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);

        // Verificar si bim.sh termino solo
        if (waitpid(pid, NULL, WNOHANG) == pid) corriendo = 0;
    }

    SDL_StopTextInput();
    kill(pid, SIGTERM);
    waitpid(pid, NULL, 0);
    close(master_fd);
    return 0;
}  // cierra screenLvLEditor
#endif // _WIN32








