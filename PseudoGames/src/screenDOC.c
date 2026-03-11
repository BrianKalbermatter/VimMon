#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include "ui.h"
#include "niveles.h"

// Calcula cuántos píxeles de alto ocupa un texto con word wrap en max_ancho
static int
altura_wrap(TTF_Font *fuente, const char *texto, int max_ancho)
{
    char copia[256];
    strncpy(copia, texto, 255);
    copia[255] = '\0';

    int alto_linea = TTF_FontHeight(fuente) + 4;
    char linea[256] = "";
    int  lineas = 0;

    char *palabra = strtok(copia, " \n\r\t");
    while (palabra) {
        char prueba[256];
        snprintf(prueba, sizeof(prueba), "%s%s%s",
                 linea, linea[0] ? " " : "", palabra);
        int w;
        TTF_SizeUTF8(fuente, prueba, &w, NULL);
        if (w > max_ancho && linea[0]) {
            lineas++;
            snprintf(linea, sizeof(linea), "%s", palabra);
        } else {
            snprintf(linea, sizeof(linea), "%s", prueba);
        }
        palabra = strtok(NULL, " \n\r\t");
    }
    if (linea[0]) lineas++;
    return lineas * alto_linea;
}

int
screenDoc(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto){
    // fgets(str, count, stream);
    //      - Cuando llamo a fgets();, lee una linea y la guarda en una variable designada
    //      - Necesita los 3 parametros:
    //              En la practica los memorizaria rapido porque siempre se ven iguales:
    //              fgets(donde_guardo, tamano_del_donde, de_donde_leo);
    //          stdin - en vez de leer de un archivo, lee lo que el usuario tipea:
    //          fgets(linea, 512, stdin); // lee del teclado
    //          fgets(); // lee de un archivo
    //
    //          char linea[512];
    //          fgets(linea, 512, archivo);
    //          - linea — el array donde guarda lo que leyó
    //          - 512 — el máximo de caracteres que puede leer (para no pasarse del array)
    //          - archivo — el archivo que está leyendo
    
    // Estado de Navegacion
    int capitulo_sel  = 0;
    int episodio_sel  = 0;   // episodio seleccionado dentro del capítulo
    int scroll        = 0;
    int vista         = 0;   // 0 = esquema, 1 = contenido del episodio
    SDL_Event evento;


    // Datos del archivo
    // el char lineas 3200 revienta porque es 1.6MB en el stack y explota fue cambiado por static
    static char lineas[3200][512];
    // Porque pasa esto?
    // En C, las variables locales viven en el stack. El stack tiene un limite chico, aproximadamente en Linux
    // Con static la variable vive en el data segment - una zona de memoria sin ese limite chico.
    // Única diferencia práctica: los static locales mantienen su valor entre llamadas
    // a la función. Para vos no importa porque total_lineas = 0 la reseteás vos mismo cada vez.
    

    int total_lineas =0;
    
    // Indice de capitulos
    typedef struct {
        char titulo[256];
        int linea_inicio;
    } Capitulo;
    
    Capitulo capitulos[20];
    int total_capitulos = 0;


    // Leer el archivo
    // Abre el archivo en modo lectura ("r"). Devuelve NULL si no lo encuentra.
    FILE *f = fopen("data/wiki.txt", "r");
    printf("DEBUG fopen: %p\n", (void*)f);
    fflush(stdout);

    // perror imprime el error real del sistema operativo. 
    if (f == NULL) { perror("wiki"); return 0;}
    
    // Cada vuelta lee una linea y la guarda en lineas[0], lineas[1], etc. Cuando no quedan mas lineas, fgets devuelve NULL y para.
    while (fgets(lineas[total_lineas], 512, f) != NULL){
        total_lineas++;
    }
    // Siempre cerrar el archivo cuando termina de verlo.
    fclose(f);

    // --- detectar capítulos ---
    for (int i = 0; i < total_lineas; i++) {
        if (strncmp(lineas[i], "CAPITULO", 8) == 0) {
            strncpy(capitulos[total_capitulos].titulo, lineas[i], 255);
            int tl = (int)strlen(capitulos[total_capitulos].titulo);
            if (tl > 0 && capitulos[total_capitulos].titulo[tl-1] == '\n')
                capitulos[total_capitulos].titulo[tl-1] = '\0';
            capitulos[total_capitulos].linea_inicio = i;
            total_capitulos++;
        }
    }

    // --- detectar episodios ---
    typedef struct {
        char titulo[256];
        int  linea_inicio;
        int  capitulo_idx;
    } Episodio;

    Episodio episodios[200];
    int total_episodios = 0;

    for (int i = 0; i < total_lineas; i++) {
        // buscar línea que empiece con espacios + "EPISODIO"
        char *p = lineas[i];
        while (*p == ' ') p++;
        if (strncmp(p, "EPISODIO", 8) != 0) continue;

        // encontrar a qué capítulo pertenece (el último que empezó antes de esta línea)
        int cap_idx = 0;
        for (int c = total_capitulos - 1; c >= 0; c--) {
            if (i >= capitulos[c].linea_inicio) { cap_idx = c; break; }
        }

        strncpy(episodios[total_episodios].titulo, p, 255);
        int tl = (int)strlen(episodios[total_episodios].titulo);
        if (tl > 0 && episodios[total_episodios].titulo[tl-1] == '\n')
            episodios[total_episodios].titulo[tl-1] = '\0';
        episodios[total_episodios].linea_inicio = i;
        episodios[total_episodios].capitulo_idx = cap_idx;
        total_episodios++;
    }
    int panel_izq_w     = 300;
    int scrollbar_w     = 14;
    int contenido_x     = panel_izq_w + 1;
    int contenido_w     = ancho - panel_izq_w - scrollbar_w - 4;
    int alto_linea      = TTF_FontHeight(fuente) + 4;
    int bar_h           = TTF_FontHeight(fuente) + 4;
    int lineas_visibles = (alto - bar_h) / alto_linea;
    int foco            = 0;  // 0 = panel capítulos, 1 = panel derecho

    while (1) {
        int clicked = 0, click_x = 0, click_y = 0;

        // --- Episodios del capítulo actual ---
        int ep_base  = -1;
        int ep_count = 0;
        for (int i = 0; i < total_episodios; i++) {
            if (episodios[i].capitulo_idx == capitulo_sel) {
                if (ep_base == -1) ep_base = i;
                ep_count++;
            }
        }
        if (ep_count > 0 && episodio_sel >= ep_count) episodio_sel = ep_count - 1;

        // Rango de líneas del episodio seleccionado (para CONTENIDO)
        int ep_global      = (ep_base >= 0) ? ep_base + episodio_sel : -1;
        int ep_linea_desde = (ep_global >= 0) ? episodios[ep_global].linea_inicio : 0;
        int cap_fin        = (capitulo_sel + 1 < total_capitulos)
                             ? capitulos[capitulo_sel + 1].linea_inicio : total_lineas;
        int ep_linea_hasta = cap_fin;
        if (ep_global >= 0 && ep_global + 1 < total_episodios
            && episodios[ep_global + 1].capitulo_idx == capitulo_sel)
            ep_linea_hasta = episodios[ep_global + 1].linea_inicio;

        int total_ep_lineas = ep_linea_hasta - ep_linea_desde;
        int max_scroll      = total_ep_lineas - lineas_visibles;
        if (max_scroll < 0) max_scroll = 0;

        // --- Eventos ---
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                SDL_RenderSetClipRect(renderer, NULL);
                return 0;
            }

            if (evento.type == SDL_KEYDOWN) {
                SDL_Keycode k = evento.key.keysym.sym;

                if (vista == 1) {
                    // --- VISTA CONTENIDO ---
                    if (k == SDLK_ESCAPE)              { vista = 0; scroll = 0; }
                    else if (k == SDLK_PAGEUP)         scroll -= lineas_visibles / 2;
                    else if (k == SDLK_PAGEDOWN)       scroll += lineas_visibles / 2;

                } else if (foco == 0) {
                    // --- FOCO: panel capítulos ---
                    if (k == SDLK_ESCAPE) {
                        SDL_RenderSetClipRect(renderer, NULL);
                        return 0;
                    }
                    else if (k == SDLK_UP)             { capitulo_sel--; episodio_sel = 0; scroll = 0; }
                    else if (k == SDLK_DOWN)           { capitulo_sel++; episodio_sel = 0; scroll = 0; }
                    else if (k == SDLK_RETURN || k == SDLK_RIGHT) { foco = 1; }

                } else {
                    // --- FOCO: panel esquema (episodios) ---
                    if (k == SDLK_ESCAPE || k == SDLK_LEFT) { foco = 0; }
                    else if (k == SDLK_UP)             episodio_sel--;
                    else if (k == SDLK_DOWN)           episodio_sel++;
                    else if (k == SDLK_RETURN)         { vista = 1; scroll = 0; }
                }

                // clamping capítulo y episodio
                if (capitulo_sel < 0) capitulo_sel = 0;
                if (capitulo_sel >= total_capitulos) capitulo_sel = total_capitulos - 1;
                if (episodio_sel < 0) episodio_sel = 0;
                if (ep_count > 0 && episodio_sel >= ep_count) episodio_sel = ep_count - 1;
            }

            if (evento.type == SDL_MOUSEBUTTONDOWN &&
                evento.button.button == SDL_BUTTON_LEFT) {
                clicked = 1;
                click_x = evento.button.x;
                click_y = evento.button.y;
            }

            if (evento.type == SDL_MOUSEWHEEL) {
                if (vista == 1) {
                    scroll -= evento.wheel.y * 3;
                } else {
                    // rueda en panel izquierdo → cambiar capitulo
                    if (click_x < panel_izq_w || evento.wheel.x != 0) {
                        capitulo_sel -= evento.wheel.y;
                        episodio_sel = 0; scroll = 0;
                    } else {
                        // rueda en panel derecho → cambiar episodio
                        episodio_sel -= evento.wheel.y;
                    }
                    if (capitulo_sel < 0) capitulo_sel = 0;
                    if (capitulo_sel >= total_capitulos) capitulo_sel = total_capitulos - 1;
                    if (episodio_sel < 0) episodio_sel = 0;
                }
            }
        }

        if (scroll < 0) scroll = 0;
        if (scroll > max_scroll) scroll = max_scroll;

        // ===================== RENDER =====================
        SDL_SetRenderDrawColor(renderer, 14, 14, 24, 255);
        SDL_RenderClear(renderer);

        // Paneles
        SDL_Rect panel_izq = {0, 0, panel_izq_w, alto};
        SDL_SetRenderDrawColor(renderer, 22, 22, 40, 255);
        SDL_RenderFillRect(renderer, &panel_izq);

        SDL_Rect panel_der = {contenido_x, 0, contenido_w + scrollbar_w + 4, alto};
        SDL_SetRenderDrawColor(renderer, 14, 14, 24, 255);
        SDL_RenderFillRect(renderer, &panel_der);

        // Divisoria (más brillante si el foco está en el panel derecho)
        SDL_SetRenderDrawColor(renderer, foco == 1 ? 120 : 60, 60, foco == 1 ? 200 : 100, 255);
        SDL_RenderDrawLine(renderer, panel_izq_w, 0, panel_izq_w, alto);

        // ---- PANEL IZQUIERDO: lista de capítulos ----
        int idx_max_w = panel_izq_w - 20;
        SDL_RenderSetClipRect(renderer, &(SDL_Rect){0, 0, panel_izq_w, alto});

        int item_y = 10;
        for (int i = 0; i < total_capitulos; i++) {
            int item_h = altura_wrap(fuente, capitulos[i].titulo, idx_max_w);
            if (item_h < alto_linea) item_h = alto_linea;

            int sel = (i == capitulo_sel);
            if (sel) {
                SDL_SetRenderDrawColor(renderer, foco==0 ? 50 : 35, 35, foco==0 ? 110 : 75, 255);
                SDL_Rect hl = {4, item_y - 2, panel_izq_w - 8, item_h + 4};
                SDL_RenderFillRect(renderer, &hl);
                SDL_SetRenderDrawColor(renderer, foco==0 ? 120 : 80, 80, foco==0 ? 220 : 140, 255);
                SDL_RenderDrawRect(renderer, &hl);
            }
            SDL_Color ci = sel ? (SDL_Color){210, 210, 255, 255} : (SDL_Color){110, 110, 160, 255};
            dibujadoTextoMultilineaColor(renderer, fuente, capitulos[i].titulo,
                                         6, item_y - 12, idx_max_w, ci);

            // click en este capitulo
            if (clicked && click_x >= 0 && click_x < panel_izq_w &&
                click_y >= item_y - 2 && click_y < item_y + item_h + 6) {
                capitulo_sel = i;
                episodio_sel = 0;
                scroll = 0;
                foco = 0;
                vista = 0;
            }

            item_y += item_h + 8;
        }

        // ---- PANEL DERECHO ----
        SDL_RenderSetClipRect(renderer, &(SDL_Rect){contenido_x, 0, contenido_w, alto});

        if (vista == 0) {
            // ---- ESQUEMA: nombre del capítulo + lista de episodios ----

            // Caja del nombre del capítulo
            int caja_y = 20;
            int caja_h = altura_wrap(fuente, capitulos[capitulo_sel].titulo, contenido_w - 30) + 20;
            SDL_Rect caja = {contenido_x + 10, caja_y, contenido_w - 20, caja_h};
            SDL_SetRenderDrawColor(renderer, 30, 30, 60, 255);
            SDL_RenderFillRect(renderer, &caja);
            SDL_SetRenderDrawColor(renderer, 100, 100, 200, 255);
            SDL_RenderDrawRect(renderer, &caja);

            SDL_Color c_titulo = {180, 180, 255, 255};
            dibujadoTextoMultilineaColor(renderer, fuente, capitulos[capitulo_sel].titulo,
                                         contenido_x + 14, caja_y + 2,
                                         contenido_w - 30, c_titulo);

            // Etiqueta "Episodios:"
            int ey = caja_y + caja_h + 20;
            SDL_Color c_label = {80, 160, 100, 255};
            dibujadoTextoColor(renderer, fuente, "Episodios:", contenido_x + 6, ey - 12, c_label);
            SDL_SetRenderDrawColor(renderer, 50, 100, 70, 255);
            SDL_RenderDrawLine(renderer, contenido_x + 10, ey + 22,
                               contenido_x + contenido_w - 10, ey + 22);
            ey += 30;

            // Lista de episodios
            for (int i = 0; i < ep_count; i++) {
                int ei       = ep_base + i;
                int sel_ep   = (i == episodio_sel && foco == 1);
                int ep_h     = altura_wrap(fuente, episodios[ei].titulo, contenido_w - 40);
                if (ep_h < alto_linea) ep_h = alto_linea;

                if (sel_ep) {
                    SDL_Rect hl = {contenido_x + 8, ey - 2, contenido_w - 16, ep_h + 6};
                    SDL_SetRenderDrawColor(renderer, 30, 60, 50, 255);
                    SDL_RenderFillRect(renderer, &hl);
                    SDL_SetRenderDrawColor(renderer, 60, 180, 100, 255);
                    SDL_RenderDrawRect(renderer, &hl);
                }

                // flecha indicadora
                SDL_Color c_arrow = sel_ep ? (SDL_Color){80, 255, 130, 255}
                                           : (SDL_Color){50, 130, 80, 255};
                dibujadoTextoColor(renderer, fuente, sel_ep ? ">" : " ",
                                   contenido_x + 10, ey - 12, c_arrow);

                SDL_Color c_ep = sel_ep ? (SDL_Color){180, 255, 200, 255}
                                        : (SDL_Color){130, 180, 150, 255};
                dibujadoTextoMultilineaColor(renderer, fuente, episodios[ei].titulo,
                                             contenido_x + 24, ey - 12,
                                             contenido_w - 40, c_ep);

                // click en este episodio → abrirlo
                if (clicked && click_x >= contenido_x && click_x < contenido_x + contenido_w &&
                    click_y >= ey - 2 && click_y < ey + ep_h + 4) {
                    episodio_sel = i;
                    foco = 1;
                    vista = 1;
                    scroll = 0;
                }

                ey += ep_h + 6;
            }

            /* hints → se dibujan en la barra de abajo, ver al final del frame */

        } else {
            // ---- CONTENIDO del episodio ----
            SDL_Color c_txt  = {195, 210, 195, 255};
            SDL_Color c_head = {90, 210, 120, 255};

            int y_txt = 10;
            for (int i = ep_linea_desde + scroll; i < ep_linea_hasta; i++) {
                if (y_txt > alto) break;
                char *lin = lineas[i];
                int len = (int)strlen(lin);
                if (len > 0 && lin[len-1] == '\n') lin[len-1] = '\0';

                if (lin[0] == '\0') { y_txt += alto_linea / 2; continue; }

                char *tr = lin; while (*tr == ' ') tr++;
                SDL_Color c = (strncmp(tr, "EPISODIO", 8) == 0 || lin[0] == '=')
                              ? c_head : c_txt;
                dibujadoTextoMultilineaColor(renderer, fuente, lin,
                                             contenido_x + 4, y_txt - 12,
                                             contenido_w - 10, c);
                y_txt += alto_linea;
            }

            // Scrollbar
            SDL_RenderSetClipRect(renderer, NULL);
            int sb_x = ancho - scrollbar_w - 2;
            SDL_Rect track = {sb_x, 4, scrollbar_w, alto - 8};
            SDL_SetRenderDrawColor(renderer, 30, 30, 50, 255);
            SDL_RenderFillRect(renderer, &track);
            SDL_SetRenderDrawColor(renderer, 55, 55, 85, 255);
            SDL_RenderDrawRect(renderer, &track);

            if (total_ep_lineas > 0) {
                float ratio = (float)lineas_visibles / total_ep_lineas;
                if (ratio > 1.0f) ratio = 1.0f;
                int th = (int)((alto - 8) * ratio);
                if (th < 20) th = 20;
                int ty = 4 + (max_scroll > 0
                         ? (int)((float)scroll / max_scroll * (alto - 8 - th)) : 0);
                SDL_Rect thumb = {sb_x + 2, ty, scrollbar_w - 4, th};
                SDL_SetRenderDrawColor(renderer, 90, 90, 170, 255);
                SDL_RenderFillRect(renderer, &thumb);

                // click en la scrollbar → saltar a esa posicion
                if (clicked && click_x >= sb_x && click_x <= sb_x + scrollbar_w &&
                    click_y >= 4 && click_y <= alto - 4 && max_scroll > 0) {
                    float t = (float)(click_y - 4) / (alto - 8);
                    scroll = (int)(t * max_scroll);
                }
            }

            /* hint → barra de abajo */
        }

        SDL_RenderSetClipRect(renderer, NULL);

        /* ── Barra de atajos fija abajo ── */
        {
            int by = alto - bar_h;
            SDL_Rect bar = {0, by, ancho, bar_h};
            SDL_SetRenderDrawColor(renderer, 15, 15, 28, 255);
            SDL_RenderFillRect(renderer, &bar);
            SDL_SetRenderDrawColor(renderer, 50, 50, 80, 255);
            SDL_RenderDrawLine(renderer, 0, by, ancho, by);

            const char *hint = (vista == 0)
                ? (foco == 0 ? "[ENTER / →] entrar   [ESC] salir"
                             : "[↑↓] navegar   [ENTER] abrir   [← / ESC] volver")
                : "[PageUp/Down / rueda] scroll   [ESC] volver al esquema";

            SDL_Color c_hint = {100, 100, 160, 255};
            SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, hint, c_hint);
            if (s) {
                SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {10, by + 2, s->w, s->h};
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    
    return 0;
}

