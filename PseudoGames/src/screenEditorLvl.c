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
int
screenLvLs(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto){
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
    int line_h  = TTF_FontHeight(fuente) + 1;
    int char_w  = 9; // aproximacion para fuente monospace a 16pt
    int bim_w   = (ancho / 2) / char_w;
    int bim_h   = alto / line_h;
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

                // F5 = marcar nivel como completado y salir
                if (k == SDLK_F5) { marcar_completado(nivel_num); corriendo = 0; break; }
                // F10 = salir sin completar (ESC lo maneja bim para cambiar de modo)
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

        // Panel derecho - bim
        SDL_Rect panel_der = {panel_x, 0, panel_w, alto};
        SDL_SetRenderDrawColor(renderer, 12, 12, 12, 255);
        SDL_RenderFillRect(renderer, &panel_der);

        // Linea divisoria
        SDL_SetRenderDrawColor(renderer, 0, 100, 40, 255);
        SDL_RenderDrawLine(renderer, panel_x, 0, panel_x, alto);

        // Consigna (panel izquierdo)
        SDL_Color verde_titulo = {0, 230, 80, 255};
        dibujadoTextoColor(renderer, fuente, n->titulo, 10, 8, verde_titulo);
        SDL_SetRenderDrawColor(renderer, 0, 130, 50, 255);
        SDL_RenderDrawLine(renderer, 10, 42, panel_x - 10, 42);
        SDL_Color verde_texto = {0, 190, 70, 255};
        dibujadoTextoMultilineaColor(renderer, fuente, n->enunciado,
                                     10, 44, panel_x - 30, verde_texto);

        // Ayuda abajo a la izquierda
        SDL_Color c_help = {50, 90, 60, 255};
        dibujadoTextoColor(renderer, fuente, "[F5] completar nivel   [F10] volver sin completar", 10, alto - 30, c_help);

        // Render del VT de bim.sh en panel derecho con clip
        SDL_RenderSetClipRect(renderer, &panel_der);

        for (int row = 0; row < bim_h && row < BIM_ROWS; row++) {
            int y = row * line_h;
            if (y > alto) break;

            // trim espacios al final
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

        // --- Cursor parpadeante verde en panel bim ---
        if ((SDL_GetTicks() / 500) % 2 == 0) {
            int cur_x = panel_x + 4 + bim_col * char_w;
            int cur_y = bim_row * line_h;
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 255, 80, 180);
            SDL_Rect cur_rect = {cur_x, cur_y, char_w, line_h};
            SDL_RenderFillRect(renderer, &cur_rect);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_RenderSetClipRect(renderer, NULL);
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










int
screenFreeEditor(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto, int nivel_num){
    return 0;
}

