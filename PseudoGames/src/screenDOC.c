#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ui.h"
#include "cJSON.h"
#include "progreso.h"
#include "screenTutorial.h"

// =============================================================
//  HIGHLIGHTS  (saves/highlights.json)
// =============================================================

#define MAX_HIGHLIGHTS 500
#define NUM_COLORES    8

typedef struct {
    int linea_absoluta;
    int char_ini;
    int char_fin;
    int color_idx;
} Highlight;

static Highlight highlights[MAX_HIGHLIGHTS];
static int       total_highlights = 0;

// RGBA — alpha alto para que el rect se vea como marcador real
static SDL_Color paleta[NUM_COLORES] = {
    {255, 230,  50, 190},  // 0 Amarillo
    { 60, 220,  90, 180},  // 1 Verde
    { 60, 190, 220, 180},  // 2 Cyan
    {220,  70, 140, 180},  // 3 Rosa
    {230, 130,  40, 180},  // 4 Naranja
    {160,  70, 220, 180},  // 5 Violeta
    {220,  60,  60, 180},  // 6 Rojo
    {255, 255, 255, 140},  // 7 Blanco
};

static int
buscar_highlight(int linea, int cs, int ce)
{
    for (int i = 0; i < total_highlights; i++)
        if (highlights[i].linea_absoluta == linea &&
            highlights[i].char_ini == cs &&
            highlights[i].char_fin == ce) return i;
    return -1;
}

static void
guardar_highlights(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *arr  = cJSON_CreateArray();
    for (int i = 0; i < total_highlights; i++) {
        cJSON *item = cJSON_CreateObject();
        cJSON_AddNumberToObject(item, "linea", highlights[i].linea_absoluta);
        cJSON_AddNumberToObject(item, "ini",   highlights[i].char_ini);
        cJSON_AddNumberToObject(item, "fin",   highlights[i].char_fin);
        cJSON_AddNumberToObject(item, "color", highlights[i].color_idx);
        cJSON_AddItemToArray(arr, item);
    }
    cJSON_AddItemToObject(root, "highlights", arr);
    char *json = cJSON_Print(root);
    cJSON_Delete(root);
    FILE *f = fopen("saves/highlights.json", "w");
    if (f) { fputs(json, f); fclose(f); }
    free(json);
}

static void
cargar_highlights(void)
{
    total_highlights = 0;
    FILE *f = fopen("saves/highlights.json", "r");
    if (!f) return;
    fseek(f, 0, SEEK_END); long len = ftell(f); fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    if (!buf) { fclose(f); return; }
    fread(buf, 1, len, f); buf[len] = '\0'; fclose(f);
    cJSON *root = cJSON_Parse(buf); free(buf);
    if (!root) return;
    cJSON *arr = cJSON_GetObjectItem(root, "highlights");
    if (cJSON_IsArray(arr)) {
        cJSON *item;
        cJSON_ArrayForEach(item, arr) {
            if (total_highlights >= MAX_HIGHLIGHTS) break;
            cJSON *l  = cJSON_GetObjectItem(item, "linea");
            cJSON *ci = cJSON_GetObjectItem(item, "ini");
            cJSON *cf = cJSON_GetObjectItem(item, "fin");
            cJSON *cc = cJSON_GetObjectItem(item, "color");
            if (cJSON_IsNumber(l) && cJSON_IsNumber(ci) &&
                cJSON_IsNumber(cf) && cJSON_IsNumber(cc)) {
                highlights[total_highlights].linea_absoluta = l->valueint;
                highlights[total_highlights].char_ini       = ci->valueint;
                highlights[total_highlights].char_fin       = cf->valueint;
                highlights[total_highlights].color_idx      = cc->valueint;
                total_highlights++;
            }
        }
    }
    cJSON_Delete(root);
}

static void
quitar_highlight(int linea, int cs, int ce)
{
    int idx = buscar_highlight(linea, cs, ce);
    if (idx < 0) return;
    for (int i = idx; i < total_highlights - 1; i++)
        highlights[i] = highlights[i + 1];
    total_highlights--;
    guardar_highlights();
}

static void
aplicar_highlight(int linea, int cs, int ce, int color_idx)
{
    int idx = buscar_highlight(linea, cs, ce);
    if (idx >= 0) {
        highlights[idx].color_idx = color_idx;
    } else if (total_highlights < MAX_HIGHLIGHTS) {
        highlights[total_highlights].linea_absoluta = linea;
        highlights[total_highlights].char_ini       = cs;
        highlights[total_highlights].char_fin       = ce;
        highlights[total_highlights].color_idx      = color_idx;
        total_highlights++;
    }
    guardar_highlights();
}

// =============================================================
//  HELPERS DE TEXTO
// =============================================================

// Convierte píxeles X desde el inicio del texto → índice de carácter
static int
pixel_a_char(TTF_Font *fuente, const char *texto, int px_rel)
{
    int len = (int)strlen(texto);
    for (int i = 1; i <= len; i++) {
        char tmp[512];
        strncpy(tmp, texto, i);
        tmp[i] = '\0';
        int w;
        TTF_SizeUTF8(fuente, tmp, &w, NULL);
        if (w > px_rel) return i - 1;
    }
    return len;
}

// Limpia el '\n' final de una línea (in-place)
static void
quitar_newline(char *s)
{
    int tl = (int)strlen(s);
    if (tl > 0 && s[tl-1] == '\n') s[tl-1] = '\0';
}

// Renderiza texto en coordenadas exactas de píxel (sin offsets internos)
static void
render_texto_xy(SDL_Renderer *renderer, TTF_Font *fuente,
                const char *texto, int x, int y, SDL_Color color)
{
    if (!texto || !texto[0]) return;
    SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, texto, color);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_Rect r = {x, y, s->w, s->h};
    SDL_RenderCopy(renderer, t, NULL, &r);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}

static int
altura_wrap(TTF_Font *fuente, const char *texto, int max_ancho)
{
    char copia[256];
    strncpy(copia, texto, 255); copia[255] = '\0';
    int al = TTF_FontHeight(fuente) + 4;
    char linea[256] = "";
    int  cnt = 0;
    char *palabra = strtok(copia, " \n\r\t");
    while (palabra) {
        char prueba[256];
        snprintf(prueba, sizeof(prueba), "%s%s%s",
                 linea, linea[0] ? " " : "", palabra);
        int w; TTF_SizeUTF8(fuente, prueba, &w, NULL);
        if (w > max_ancho && linea[0]) { cnt++; snprintf(linea, sizeof(linea), "%s", palabra); }
        else                            snprintf(linea, sizeof(linea), "%s", prueba);
        palabra = strtok(NULL, " \n\r\t");
    }
    if (linea[0]) cnt++;
    return cnt * al;
}

// =============================================================
//  PANTALLA DOC
// =============================================================

int
screenDoc(SDL_Renderer *renderer, TTF_Font *fuente, int ancho, int alto)
{
    if (!tutorial_ya_visto()) {
        screenTutorial(renderer, fuente, ancho, alto);
        marcar_tutorial_visto();
    }

    cargar_highlights();

    int capitulo_sel  = 0;
    int episodio_sel  = 0;
    int scroll        = 0;
    int vista         = 0;
    SDL_Event evento;

    // Selección de texto
    int sel_activa      = 0;
    int sel_arrastrando = 0;
    int sel_lin_ini = -1, sel_char_ini = 0;
    int sel_lin_fin = -1, sel_char_fin = 0;

    // Popup de colores
    int popup_visible = 0;
    int popup_x = 0, popup_y = 0;
    int popup_lin = -1, popup_cs = 0, popup_ce = 0;

    int sq  = 22;   // tamaño cuadro de color
    int gap = 4;
    int pad = 8;
    int popup_w = pad * 2 + NUM_COLORES * sq + (NUM_COLORES - 1) * gap + gap + sq; // +gap+sq para botón X
    int popup_h = pad * 2 + sq;

    static char lineas[3200][512];
    int total_lineas = 0;

    typedef struct { char titulo[256]; int linea_inicio; } Capitulo;
    Capitulo capitulos[20];
    int total_capitulos = 0;

    FILE *f = fopen("data/wiki.txt", "r");
    if (!f) { perror("wiki"); return 0; }
    while (fgets(lineas[total_lineas], 512, f) != NULL) total_lineas++;
    fclose(f);

    for (int i = 0; i < total_lineas; i++) {
        if (strncmp(lineas[i], "CAPITULO", 8) == 0) {
            strncpy(capitulos[total_capitulos].titulo, lineas[i], 255);
            quitar_newline(capitulos[total_capitulos].titulo);
            capitulos[total_capitulos].linea_inicio = i;
            total_capitulos++;
        }
    }

    typedef struct { char titulo[256]; int linea_inicio; int capitulo_idx; } Episodio;
    Episodio episodios[200];
    int total_episodios = 0;
    for (int i = 0; i < total_lineas; i++) {
        char *p = lineas[i]; while (*p == ' ') p++;
        if (strncmp(p, "EPISODIO", 8) != 0) continue;
        int cap_idx = 0;
        for (int c = total_capitulos-1; c >= 0; c--)
            if (i >= capitulos[c].linea_inicio) { cap_idx = c; break; }
        strncpy(episodios[total_episodios].titulo, p, 255);
        quitar_newline(episodios[total_episodios].titulo);
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
    int foco            = 0;

    // dibujadoTextoColor agrega +10 a x y +12 a y internamente.
    // texto_param_x  = lo que le paso a la función dibujado
    // texto_real_x   = donde el texto REALMENTE aparece en pantalla
    int texto_param_x = contenido_x + 6;
    int texto_real_x  = texto_param_x + 10;   // +10 del helper

    // line_y[i] = Y real en pantalla donde empieza la línea i del archivo
    static int line_y[3200];

    while (1) {
        int clicked = 0, click_x = 0, click_y = 0;

        // Episodios del capítulo actual
        int ep_base = -1, ep_count = 0;
        for (int i = 0; i < total_episodios; i++) {
            if (episodios[i].capitulo_idx == capitulo_sel) {
                if (ep_base == -1) ep_base = i;
                ep_count++;
            }
        }
        if (ep_count > 0 && episodio_sel >= ep_count) episodio_sel = ep_count - 1;

        int ep_global      = (ep_base >= 0) ? ep_base + episodio_sel : -1;
        int ep_linea_desde = (ep_global >= 0) ? episodios[ep_global].linea_inicio : 0;
        int cap_fin        = (capitulo_sel+1 < total_capitulos)
                             ? capitulos[capitulo_sel+1].linea_inicio : total_lineas;
        int ep_linea_hasta = cap_fin;
        if (ep_global >= 0 && ep_global+1 < total_episodios
            && episodios[ep_global+1].capitulo_idx == capitulo_sel)
            ep_linea_hasta = episodios[ep_global+1].linea_inicio;

        int total_ep_lineas = ep_linea_hasta - ep_linea_desde;
        int max_scroll      = total_ep_lineas - lineas_visibles;
        if (max_scroll < 0) max_scroll = 0;

        // ── Precalcular Y real de cada línea (respeta líneas vacías) ──
        // Esto es lo que soluciona el offset: usamos la misma lógica
        // que el render, así click y render siempre coinciden.
        {
            int y = 10;
            for (int i = ep_linea_desde + scroll; i < ep_linea_hasta; i++) {
                line_y[i] = y;
                char tmp[512];
                strncpy(tmp, lineas[i], 511); tmp[511] = '\0';
                quitar_newline(tmp);
                y += (tmp[0] == '\0') ? alto_linea / 2 : alto_linea;
            }
        }

        // ── Convertir Y de mouse → índice de línea ──
        // Busca en line_y[] cuál es la línea más cercana al click
        #define MY_A_LINEA(my) ({                                          \
            int _r = ep_linea_desde + scroll;                              \
            for (int _i = ep_linea_desde+scroll; _i < ep_linea_hasta-1; _i++) { \
                if ((my) >= line_y[_i] && (my) < line_y[_i+1]) { _r=_i; break; } \
                _r = ep_linea_hasta - 1;                                   \
            } _r; })

        // ── Convertir X de mouse → índice de carácter ──
        #define MX_A_CHAR(mx, linea_abs) ({                               \
            char _tmp[512]; strncpy(_tmp, lineas[linea_abs], 511);        \
            _tmp[511] = '\0'; quitar_newline(_tmp);                       \
            int _px = (mx) - texto_real_x; if (_px < 0) _px = 0;        \
            pixel_a_char(fuente, _tmp, _px); })

        // ── EVENTOS ──
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) {
                SDL_RenderSetClipRect(renderer, NULL); return 0;
            }

            if (evento.type == SDL_MOUSEBUTTONDOWN &&
                evento.button.button == SDL_BUTTON_LEFT) {
                clicked = 1; click_x = evento.button.x; click_y = evento.button.y;

                // ── Click dentro del popup → aplicar color (tiene prioridad) ──
                if (popup_visible &&
                    click_x >= popup_x && click_x < popup_x + popup_w &&
                    click_y >= popup_y && click_y < popup_y + popup_h) {

                    for (int ci = 0; ci < NUM_COLORES; ci++) {
                        int sx = popup_x + pad + ci * (sq + gap);
                        int sy = popup_y + pad;
                        if (click_x >= sx && click_x < sx + sq &&
                            click_y >= sy && click_y < sy + sq) {
                            aplicar_highlight(popup_lin, popup_cs, popup_ce, ci);
                            break;
                        }
                    }
                    // botón eliminar (X) — último cuadro del popup
                    {
                        int del_x = popup_x + pad + NUM_COLORES * (sq + gap);
                        int del_y = popup_y + pad;
                        if (click_x >= del_x && click_x < del_x + sq &&
                            click_y >= del_y && click_y < del_y + sq) {
                            quitar_highlight(popup_lin, popup_cs, popup_ce);
                        }
                    }
                    popup_visible = 0;
                    sel_activa    = 0;
                    sel_lin_ini   = -1;
                    sel_lin_fin   = -1;
                    sel_char_ini  = 0;    // evita que MOUSEBUTTONUP reactive el popup
                    sel_char_fin  = 0;

                // ── Click fuera del popup → cerrarlo ──
                } else if (popup_visible) {
                    popup_visible = 0;
                    sel_activa    = 0;
                    sel_lin_ini   = -1;
                    sel_lin_fin   = -1;
                    sel_char_ini  = 0;
                    sel_char_fin  = 0;

                // ── Click en área de contenido → iniciar selección ──
                } else if (vista == 1 &&
                    click_x >= contenido_x && click_x < contenido_x + contenido_w &&
                    click_y >= 0 && click_y < alto - bar_h) {
                    sel_activa = 0; sel_arrastrando = 1;
                    sel_lin_ini  = MY_A_LINEA(click_y);
                    sel_char_ini = MX_A_CHAR(click_x, sel_lin_ini);
                    sel_lin_fin  = sel_lin_ini;
                    sel_char_fin = sel_char_ini;
                }
            }

            if (evento.type == SDL_MOUSEMOTION && sel_arrastrando) {
                int mx = evento.motion.x, my = evento.motion.y;
                if (mx >= contenido_x && mx < contenido_x + contenido_w &&
                    my >= 0 && my < alto - bar_h) {
                    sel_lin_fin  = MY_A_LINEA(my);
                    sel_char_fin = MX_A_CHAR(mx, sel_lin_fin);
                    sel_activa   = (sel_lin_ini != sel_lin_fin ||
                                    sel_char_ini != sel_char_fin);
                }
            }

            if (evento.type == SDL_MOUSEBUTTONUP &&
                evento.button.button == SDL_BUTTON_LEFT) {
                sel_arrastrando = 0;
                sel_activa = (sel_lin_ini != sel_lin_fin ||
                              sel_char_ini != sel_char_fin);

                if (sel_activa && vista == 1) {
                    // Normalizar
                    int lin_lo, chr_lo, lin_hi, chr_hi;
                    if (sel_lin_ini < sel_lin_fin ||
                        (sel_lin_ini == sel_lin_fin && sel_char_ini <= sel_char_fin)) {
                        lin_lo = sel_lin_ini; chr_lo = sel_char_ini;
                        lin_hi = sel_lin_fin; chr_hi = sel_char_fin;
                    } else {
                        lin_lo = sel_lin_fin; chr_lo = sel_char_fin;
                        lin_hi = sel_lin_ini; chr_hi = sel_char_ini;
                    }
                    popup_lin = lin_lo;
                    popup_cs  = chr_lo;
                    popup_ce  = (lin_lo == lin_hi) ? chr_hi : (int)strlen(lineas[lin_lo]);

                    // Popup justo arriba del cursor al soltar
                    popup_x = evento.button.x;
                    popup_y = evento.button.y - popup_h - 8;
                    if (popup_y < 4)              popup_y = evento.button.y + 10;
                    if (popup_x + popup_w > ancho) popup_x = ancho - popup_w - 4;
                    popup_visible = 1;
                }
            }

            if (evento.type == SDL_KEYDOWN) {
                SDL_Keycode k = evento.key.keysym.sym;
                if (vista == 1) {
                    if (k == SDLK_ESCAPE) {
                        if (popup_visible) { popup_visible = 0; sel_activa = 0; }
                        else               { vista = 0; scroll = 0; sel_activa = 0; }
                    }
                    else if (k == SDLK_PAGEUP)   scroll -= lineas_visibles / 2;
                    else if (k == SDLK_PAGEDOWN)  scroll += lineas_visibles / 2;
                } else if (foco == 0) {
                    if (k == SDLK_ESCAPE || k == SDLK_F9) { SDL_RenderSetClipRect(renderer, NULL); return 0; }
                    else if (k == SDLK_UP)   { capitulo_sel--; episodio_sel = 0; scroll = 0; }
                    else if (k == SDLK_DOWN) { capitulo_sel++; episodio_sel = 0; scroll = 0; }
                    else if (k == SDLK_RETURN || k == SDLK_RIGHT) foco = 1;
                } else {
                    if (k == SDLK_ESCAPE || k == SDLK_LEFT) foco = 0;
                    else if (k == SDLK_UP)     episodio_sel--;
                    else if (k == SDLK_DOWN)   episodio_sel++;
                    else if (k == SDLK_RETURN) { vista = 1; scroll = 0; }
                }
                if (capitulo_sel < 0) capitulo_sel = 0;
                if (capitulo_sel >= total_capitulos) capitulo_sel = total_capitulos-1;
                if (episodio_sel < 0) episodio_sel = 0;
                if (ep_count > 0 && episodio_sel >= ep_count) episodio_sel = ep_count-1;
            }

            if (evento.type == SDL_MOUSEWHEEL) {
                if (vista == 1) {
                    scroll -= evento.wheel.y * 3;
                    popup_visible = 0; sel_activa = 0;
                } else {
                    if (click_x < panel_izq_w || evento.wheel.x != 0) {
                        capitulo_sel -= evento.wheel.y; episodio_sel = 0; scroll = 0;
                    } else {
                        episodio_sel -= evento.wheel.y;
                    }
                    if (capitulo_sel < 0) capitulo_sel = 0;
                    if (capitulo_sel >= total_capitulos) capitulo_sel = total_capitulos-1;
                    if (episodio_sel < 0) episodio_sel = 0;
                }
            }
        }

        if (scroll < 0) scroll = 0;
        if (scroll > max_scroll) scroll = max_scroll;

        // ===================== RENDER =====================
        SDL_SetRenderDrawColor(renderer, 14, 14, 24, 255);
        SDL_RenderClear(renderer);

        // Fondos
        SDL_Rect panel_izq = {0, 0, panel_izq_w, alto};
        SDL_SetRenderDrawColor(renderer, 22, 22, 40, 255);
        SDL_RenderFillRect(renderer, &panel_izq);
        SDL_Rect panel_der = {contenido_x, 0, contenido_w + scrollbar_w + 4, alto};
        SDL_SetRenderDrawColor(renderer, 14, 14, 24, 255);
        SDL_RenderFillRect(renderer, &panel_der);
        SDL_SetRenderDrawColor(renderer, foco==1?120:60, 60, foco==1?200:100, 255);
        SDL_RenderDrawLine(renderer, panel_izq_w, 0, panel_izq_w, alto);

        // ── PANEL IZQUIERDO ──
        SDL_RenderSetClipRect(renderer, &(SDL_Rect){0, 0, panel_izq_w, alto});
        int item_y = 10;
        for (int i = 0; i < total_capitulos; i++) {
            int imw  = panel_izq_w - 20;
            int ih   = altura_wrap(fuente, capitulos[i].titulo, imw);
            if (ih < alto_linea) ih = alto_linea;
            int sel  = (i == capitulo_sel);
            if (sel) {
                SDL_SetRenderDrawColor(renderer, foco==0?50:35, 35, foco==0?110:75, 255);
                SDL_Rect hl = {4, item_y-2, panel_izq_w-8, ih+4};
                SDL_RenderFillRect(renderer, &hl);
                SDL_SetRenderDrawColor(renderer, foco==0?120:80, 80, foco==0?220:140, 255);
                SDL_RenderDrawRect(renderer, &hl);
            }
            SDL_Color ci = sel ? (SDL_Color){210,210,255,255} : (SDL_Color){110,110,160,255};
            dibujadoTextoMultilineaColor(renderer, fuente, capitulos[i].titulo,
                                         6, item_y-12, imw, ci);
            if (clicked && click_x >= 0 && click_x < panel_izq_w &&
                click_y >= item_y-2 && click_y < item_y+ih+6) {
                capitulo_sel = i; episodio_sel = 0; scroll = 0; foco = 0; vista = 0;
            }
            item_y += ih + 8;
        }

        // ── PANEL DERECHO ──
        SDL_RenderSetClipRect(renderer, &(SDL_Rect){contenido_x, 0, contenido_w, alto});

        if (vista == 0) {
            // Esquema
            int cy  = 20;
            int ch  = altura_wrap(fuente, capitulos[capitulo_sel].titulo, contenido_w-30) + 20;
            SDL_Rect caja = {contenido_x+10, cy, contenido_w-20, ch};
            SDL_SetRenderDrawColor(renderer, 30,30,60,255); SDL_RenderFillRect(renderer, &caja);
            SDL_SetRenderDrawColor(renderer, 100,100,200,255); SDL_RenderDrawRect(renderer, &caja);
            dibujadoTextoMultilineaColor(renderer, fuente, capitulos[capitulo_sel].titulo,
                                         contenido_x+14, cy+2, contenido_w-30,
                                         (SDL_Color){180,180,255,255});
            int ey = cy + ch + 20;
            dibujadoTextoColor(renderer, fuente, "Episodios:",
                               contenido_x+6, ey-12, (SDL_Color){80,160,100,255});
            SDL_SetRenderDrawColor(renderer, 50,100,70,255);
            SDL_RenderDrawLine(renderer, contenido_x+10, ey+22, contenido_x+contenido_w-10, ey+22);
            ey += 30;
            for (int i = 0; i < ep_count; i++) {
                int ei  = ep_base + i;
                int sp  = (i == episodio_sel && foco == 1);
                int eph = altura_wrap(fuente, episodios[ei].titulo, contenido_w-40);
                if (eph < alto_linea) eph = alto_linea;
                if (sp) {
                    SDL_Rect hl = {contenido_x+8, ey-2, contenido_w-16, eph+6};
                    SDL_SetRenderDrawColor(renderer,30,60,50,255); SDL_RenderFillRect(renderer,&hl);
                    SDL_SetRenderDrawColor(renderer,60,180,100,255); SDL_RenderDrawRect(renderer,&hl);
                }
                dibujadoTextoColor(renderer, fuente, sp?">":" ", contenido_x+10, ey-12,
                                   sp?(SDL_Color){80,255,130,255}:(SDL_Color){50,130,80,255});
                dibujadoTextoMultilineaColor(renderer, fuente, episodios[ei].titulo,
                                             contenido_x+24, ey-12, contenido_w-40,
                                             sp?(SDL_Color){180,255,200,255}:(SDL_Color){130,180,150,255});
                if (clicked && click_x >= contenido_x && click_x < contenido_x+contenido_w &&
                    click_y >= ey-2 && click_y < ey+eph+4) {
                    episodio_sel = i; foco = 1; vista = 1; scroll = 0; sel_activa = 0;
                }
                ey += eph + 6;
            }

        } else {
            // ── CONTENIDO ──
            SDL_Color c_txt  = {195, 210, 195, 255};
            SDL_Color c_head = { 90, 210, 120, 255};

            // Normalizar selección
            int lin_lo, lin_hi, chr_lo, chr_hi;
            if (sel_lin_ini < sel_lin_fin ||
                (sel_lin_ini == sel_lin_fin && sel_char_ini <= sel_char_fin)) {
                lin_lo = sel_lin_ini; chr_lo = sel_char_ini;
                lin_hi = sel_lin_fin; chr_hi = sel_char_fin;
            } else {
                lin_lo = sel_lin_fin; chr_lo = sel_char_fin;
                lin_hi = sel_lin_ini; chr_hi = sel_char_ini;
            }

            for (int i = ep_linea_desde + scroll; i < ep_linea_hasta; i++) {
                int y_txt = line_y[i];          // Y real calculado arriba
                if (y_txt > alto - bar_h) break;

                char *lin = lineas[i];
                int len = (int)strlen(lin);
                if (len > 0 && lin[len-1] == '\n') lin[len-1] = '\0';

                // ── Fondo de highlights guardados ──
                for (int h = 0; h < total_highlights; h++) {
                    if (highlights[h].linea_absoluta != i) continue;
                    int cs = highlights[h].char_ini;
                    int ce = highlights[h].char_fin;
                    SDL_Color hc = paleta[highlights[h].color_idx];
                    int x1 = 0, x2 = 0;
                    if (cs > 0) {
                        char part[512]; strncpy(part, lin, cs); part[cs] = '\0';
                        TTF_SizeUTF8(fuente, part, &x1, NULL);
                    }
                    if (ce > 0) {
                        char part[512]; strncpy(part, lin, ce); part[ce] = '\0';
                        TTF_SizeUTF8(fuente, part, &x2, NULL);
                    }
                    // padding de 2px arriba y abajo → parece marcador real
                    SDL_Rect hr = {texto_real_x + x1 - 2,
                                   y_txt - 2,
                                   x2 - x1 + 4,
                                   alto_linea + 2};
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, hc.r, hc.g, hc.b, hc.a);
                    SDL_RenderFillRect(renderer, &hr);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }

                // ── Fondo de selección activa ──
                if ((sel_activa || sel_arrastrando) && i >= lin_lo && i <= lin_hi) {
                    int cs, ce, ll = (int)strlen(lin);
                    if (lin_lo == lin_hi) {
                        cs = chr_lo < chr_hi ? chr_lo : chr_hi;
                        ce = chr_lo < chr_hi ? chr_hi : chr_lo;
                    } else if (i == lin_lo) { cs = chr_lo; ce = ll;    }
                    else if (i == lin_hi)   { cs = 0;      ce = chr_hi; }
                    else                    { cs = 0;      ce = ll;    }

                    int x1 = 0, x2 = 0;
                    if (cs > 0) {
                        char part[512]; strncpy(part, lin, cs); part[cs] = '\0';
                        TTF_SizeUTF8(fuente, part, &x1, NULL);
                    }
                    if (ce > 0) {
                        char part[512]; strncpy(part, lin, ce); part[ce] = '\0';
                        TTF_SizeUTF8(fuente, part, &x2, NULL);
                    }
                    if (x2 - x1 < 2 && ce > cs) x2 = x1 + 2;

                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(renderer, 70, 130, 200, 130);
                    SDL_Rect sr = {texto_real_x + x1, y_txt, x2 - x1, alto_linea - 2};
                    SDL_RenderFillRect(renderer, &sr);
                    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                }

                if (lin[0] == '\0') continue;

                // Texto
                char *tr = lin; while (*tr == ' ') tr++;
                SDL_Color c = (strncmp(tr,"EPISODIO",8)==0 || lin[0]=='=') ? c_head : c_txt;
                // Buscar si esta línea tiene highlight
                int h_idx = -1;
                for (int h = 0; h < total_highlights; h++) {
                    if (highlights[h].linea_absoluta == i) { h_idx = h; break; }
                }

                if (h_idx < 0) {
                    // Sin highlight → render normal
                    dibujadoTextoMultilineaColor(renderer, fuente, lin,
                                                 texto_param_x, y_txt - 12,
                                                 contenido_w - 10, c);
                } else {
                    // Con highlight → 3 segmentos para evitar texto doble
                    int cs = highlights[h_idx].char_ini;
                    int ce = highlights[h_idx].char_fin;
                    int ll = (int)strlen(lin);
                    SDL_Color negro = {15, 15, 15, 255};

                    // Pixel X donde empieza cada segmento
                    int x1 = 0, x2 = 0;
                    if (cs > 0) {
                        char p[512]; strncpy(p, lin, cs); p[cs] = '\0';
                        TTF_SizeUTF8(fuente, p, &x1, NULL);
                    }
                    if (ce > 0) {
                        char p[512]; strncpy(p, lin, ce); p[ce] = '\0';
                        TTF_SizeUTF8(fuente, p, &x2, NULL);
                    }

                    // Segmento 1: antes del highlight
                    if (cs > 0) {
                        char seg[512]; strncpy(seg, lin, cs); seg[cs] = '\0';
                        render_texto_xy(renderer, fuente, seg,
                                        texto_real_x, y_txt, c);
                    }
                    // Segmento 2: el highlight en negro
                    if (ce > cs) {
                        char seg[512];
                        int slen = ce - cs; if (slen > 511) slen = 511;
                        strncpy(seg, lin + cs, slen); seg[slen] = '\0';
                        render_texto_xy(renderer, fuente, seg,
                                        texto_real_x + x1, y_txt, negro);
                    }
                    // Segmento 3: después del highlight
                    if (ce < ll) {
                        char seg[512];
                        int slen = ll - ce; if (slen > 511) slen = 511;
                        strncpy(seg, lin + ce, slen); seg[slen] = '\0';
                        render_texto_xy(renderer, fuente, seg,
                                        texto_real_x + x2, y_txt, c);
                    }
                }
            }

            // Scrollbar
            SDL_RenderSetClipRect(renderer, NULL);
            int sb_x = ancho - scrollbar_w - 2;
            SDL_Rect track = {sb_x, 4, scrollbar_w, alto-8};
            SDL_SetRenderDrawColor(renderer, 30,30,50,255); SDL_RenderFillRect(renderer, &track);
            SDL_SetRenderDrawColor(renderer, 55,55,85,255); SDL_RenderDrawRect(renderer, &track);
            if (total_ep_lineas > 0) {
                float ratio = (float)lineas_visibles / total_ep_lineas;
                if (ratio > 1.0f) ratio = 1.0f;
                int th = (int)((alto-8)*ratio); if (th < 20) th = 20;
                int ty = 4 + (max_scroll > 0
                         ? (int)((float)scroll/max_scroll*(alto-8-th)) : 0);
                SDL_Rect thumb = {sb_x+2, ty, scrollbar_w-4, th};
                SDL_SetRenderDrawColor(renderer, 90,90,170,255);
                SDL_RenderFillRect(renderer, &thumb);
                if (clicked && click_x >= sb_x && click_x <= sb_x+scrollbar_w &&
                    click_y >= 4 && click_y <= alto-4 && max_scroll > 0) {
                    float t = (float)(click_y-4)/(alto-8);
                    scroll = (int)(t*max_scroll);
                }
            }

            // ── POPUP de colores ──
            if (popup_visible) {
                SDL_RenderSetClipRect(renderer, NULL);
                SDL_Rect pop = {popup_x, popup_y, popup_w, popup_h};
                SDL_SetRenderDrawColor(renderer, 20, 20, 35, 245);
                SDL_RenderFillRect(renderer, &pop);
                SDL_SetRenderDrawColor(renderer, 100, 100, 160, 255);
                SDL_RenderDrawRect(renderer, &pop);

                    int mx, my; SDL_GetMouseState(&mx, &my);

                    // cuadros de color
                    for (int ci = 0; ci < NUM_COLORES; ci++) {
                        SDL_Rect sq_r = {popup_x + pad + ci*(sq+gap), popup_y + pad, sq, sq};
                        SDL_Color c = paleta[ci];
                        SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 255);
                        SDL_RenderFillRect(renderer, &sq_r);
                        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 80);
                        SDL_RenderDrawRect(renderer, &sq_r);
                        if (mx >= sq_r.x && mx < sq_r.x+sq &&
                            my >= sq_r.y && my < sq_r.y+sq) {
                            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                            SDL_Rect outer = {sq_r.x-1, sq_r.y-1, sq_r.w+2, sq_r.h+2};
                            SDL_RenderDrawRect(renderer, &outer);
                        }
                    }

                    // botón eliminar (X)
                    {
                        int del_x = popup_x + pad + NUM_COLORES * (sq + gap);
                        int del_y = popup_y + pad;
                        SDL_Rect del_r = {del_x, del_y, sq, sq};
                        SDL_SetRenderDrawColor(renderer, 80, 25, 25, 255);
                        SDL_RenderFillRect(renderer, &del_r);
                        SDL_SetRenderDrawColor(renderer, 200, 60, 60, 255);
                        SDL_RenderDrawRect(renderer, &del_r);
                        if (mx >= del_x && mx < del_x+sq && my >= del_y && my < del_y+sq) {
                            SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
                            SDL_Rect outer = {del_r.x-1, del_r.y-1, del_r.w+2, del_r.h+2};
                            SDL_RenderDrawRect(renderer, &outer);
                        }
                        SDL_Surface *xs = TTF_RenderUTF8_Blended(fuente, "X",
                                          (SDL_Color){220, 80, 80, 255});
                        if (xs) {
                            SDL_Texture *xt = SDL_CreateTextureFromSurface(renderer, xs);
                            SDL_Rect xr = {del_x + (sq - xs->w)/2,
                                           del_y + (sq - xs->h)/2,
                                           xs->w, xs->h};
                            SDL_RenderCopy(renderer, xt, NULL, &xr);
                            SDL_FreeSurface(xs); SDL_DestroyTexture(xt);
                        }
                    }
            }
        }

        SDL_RenderSetClipRect(renderer, NULL);

        // ── Barra de atajos ──
        {
            int by = alto - bar_h;
            SDL_Rect bar = {0, by, ancho, bar_h};
            SDL_SetRenderDrawColor(renderer, 15,15,28,255); SDL_RenderFillRect(renderer, &bar);
            SDL_SetRenderDrawColor(renderer, 50,50,80,255);
            SDL_RenderDrawLine(renderer, 0, by, ancho, by);
            const char *hint = (vista == 0)
                ? (foco==0 ? "[ENTER/→] entrar  [ESC] salir"
                           : "[↑↓] navegar  [ENTER] abrir  [←/ESC] volver")
                : "[drag] seleccionar → elegir color  [PgUp/Dn] scroll  [ESC] volver";
            SDL_Color ch = {100,100,160,255};
            SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, hint, ch);
            if (s) {
                SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                SDL_Rect r = {10, by+2, s->w, s->h};
                SDL_RenderCopy(renderer, t, NULL, &r);
                SDL_FreeSurface(s); SDL_DestroyTexture(t);
            }
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);

        #undef MY_A_LINEA
        #undef MX_A_CHAR
    }

    return 0;
}
