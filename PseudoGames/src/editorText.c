#include "editorText.h"
#include "ui.h"
#include <string.h>
#include <stdio.h>

/* Dibuja texto en (x, y) exacto, sin offsets extra */
static void
dibujadoTextoSimple(SDL_Renderer *renderer, TTF_Font *fuente,
                    const char *texto, int x, int y, SDL_Color color)
{
    SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, texto, color);
    if (!s) return;
    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
    SDL_Rect pos   = { x, y, s->w, s->h };
    SDL_RenderCopy(renderer, t, NULL, &pos);
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t);
}

/* ── Paleta gruvbox dark ──────────────────────────────────────────── */
#define BG_R   40,  40,  40  /* #282828 fondo del editor              */
#define GT_R   29,  32,  33  /* #1d2021 fondo del gutter (numeros)    */
#define LN_R  102,  92,  84  /* #665c54 color de los numeros de linea */
#define PH_R   80,  73,  69  /* #504945 color del placeholder         */
#define TX_R  235, 219, 178  /* #ebdbb2 texto normal (futuro)         */

#define GUTTER_W   50   /* ancho del gutter en pixeles  */
#define LINEAS_VIS 20   /* cuantas lineas se ven        */

/* ── drawEditorText ───────────────────────────────────────────────────
 * Dibuja el editor dentro del rect `area` sin loop propio.
 * Se puede llamar desde cualquier pantalla para embeber el editor.
 * ─────────────────────────────────────────────────────────────────── */
void
drawEditorText(SDL_Renderer *renderer, TTF_Font *fuente, SDL_Rect area)
{
    int line_h = TTF_FontHeight(fuente) + 4;
    int x = area.x;
    int y = area.y;
    int h = area.h;

    SDL_RenderSetClipRect(renderer, &area);

    /* Fondo del editor */
    SDL_SetRenderDrawColor(renderer, BG_R, 255);
    SDL_RenderFillRect(renderer, &area);

    /* Gutter */
    SDL_Rect gutter = { x, y, GUTTER_W, h };
    SDL_SetRenderDrawColor(renderer, GT_R, 255);
    SDL_RenderFillRect(renderer, &gutter);

    /* Separador gutter / contenido */
    SDL_SetRenderDrawColor(renderer, LN_R, 255);
    SDL_RenderDrawLine(renderer, x + GUTTER_W, y, x + GUTTER_W, y + h);

    SDL_Color c_linea_num   = { LN_R, 255 };
    SDL_Color c_placeholder = { PH_R, 255 };
    SDL_Color c_cursor      = { TX_R, 200 };

    for (int i = 0; i < LINEAS_VIS; i++) {
        int ly = y + i * line_h + 8;
        if (ly + line_h > y + h) break;

        /* numero de linea */
        char num[8];
        snprintf(num, sizeof(num), "%d", i + 1);
        int num_w;
        TTF_SizeUTF8(fuente, num, &num_w, NULL);

        SDL_Surface *s_num = TTF_RenderUTF8_Blended(fuente, num, c_linea_num);
        if (s_num) {
            SDL_Texture *t_num = SDL_CreateTextureFromSurface(renderer, s_num);
            SDL_Rect pos = { x + GUTTER_W - num_w - 8, ly, s_num->w, s_num->h };
            SDL_RenderCopy(renderer, t_num, NULL, &pos);
            SDL_FreeSurface(s_num);
            SDL_DestroyTexture(t_num);
        }

        /* placeholder en linea 1 */
        if (i == 0) {
            const char *ph = "Escribe tu codigo aqui...";
            SDL_Surface *s_ph = TTF_RenderUTF8_Blended(fuente, ph, c_placeholder);
            if (s_ph) {
                SDL_Texture *t_ph = SDL_CreateTextureFromSurface(renderer, s_ph);
                SDL_Rect pos = { x + GUTTER_W + 12, ly, s_ph->w, s_ph->h };
                SDL_RenderCopy(renderer, t_ph, NULL, &pos);
                SDL_FreeSurface(s_ph);
                SDL_DestroyTexture(t_ph);
            }

            /* cursor fijo */
            SDL_SetRenderDrawColor(renderer, c_cursor.r, c_cursor.g, c_cursor.b, c_cursor.a);
            SDL_Rect cursor = { x + GUTTER_W + 12, ly, 2, line_h - 2 };
            SDL_RenderFillRect(renderer, &cursor);
        }
    }

    SDL_RenderSetClipRect(renderer, NULL);
}

/* ── Paleta syntax highlighting (gruvbox) ────────────────────────── */
#define C_KEYWORD  251,  73,  52, 255  /* rosa    #fb4934  SI MIENTRAS ACCION...*/
#define C_TIPO      69, 133, 136, 255  /* azul    #458588  ENTERO REAL VAR...   */
#define C_OPERADOR 184, 187,  38, 255  /* verde   #b8bb26  Y O NO               */
#define C_FUNC     250, 189,  47, 255  /* amarill #fabd2f  MOD TRUNC REDOND...  */
#define C_COMMENT  146, 131, 116, 255  /* gris    #928374  // comentarios       */
#define C_IO       142, 192, 124, 255  /* celeste #8ec07c  ESCRIBIR LEER        */
#define C_STRING   254, 128,  25, 255  /* naranja #fe8019  "strings"            */
#define C_NUMBER   211, 134, 155, 255  /* purpura #d3869b  numeros              */
#define C_NORMAL   235, 219, 178, 255  /* texto   #ebdbb2  variables y resto    */

/* Palabras clave por categoria (de sintaxis.json) */
static const char *kw_keywords[] = {
    "HACER","ABRIR","CERRAR","ARCHIVO","REGISTRO",
    "SI","FIN_SI","SINO",
    "MIENTRAS","FIN_MIENTRAS","PARA","FIN_PARA",
    "RETORNAR","FUNCION","FIN_FUNCION","SEGUN",
    "PROCEDIMIENTO","FIN_PROCEDIMIENTO",
    "ACCION","ES","FIN_ACCION","PROCESO","AMBIENTE", NULL
};
static const char *kw_tipos[] = {
    "ENTERO","REAL","CARACTER","BOOLEANO",
    "SECUENCIA_DE_CARACTERES","SECUENCIA_DE_ENTEROS",
    "ARREGLO","CONSTANTE","VAR","DE","AN","N", NULL
};
static const char *kw_operadores[] = { "Y","O","NO", NULL };
static const char *kw_funciones[]  = { "MOD","RETURN","TRUNC","ABSO","REDOND", NULL };
static const char *kw_io[]         = { "ESCRIBIR","LEER", NULL };

/* Devuelve el color segun la categoria del token */
static SDL_Color
get_token_color(const char *tok)
{
    for (int i = 0; kw_keywords[i];  i++) if (strcmp(tok, kw_keywords[i])  == 0) return (SDL_Color){C_KEYWORD};
    for (int i = 0; kw_tipos[i];     i++) if (strcmp(tok, kw_tipos[i])     == 0) return (SDL_Color){C_TIPO};
    for (int i = 0; kw_operadores[i];i++) if (strcmp(tok, kw_operadores[i]) == 0) return (SDL_Color){C_OPERADOR};
    for (int i = 0; kw_funciones[i]; i++) if (strcmp(tok, kw_funciones[i])  == 0) return (SDL_Color){C_FUNC};
    for (int i = 0; kw_io[i];        i++) if (strcmp(tok, kw_io[i])         == 0) return (SDL_Color){C_IO};
    return (SDL_Color){C_NORMAL};
}

/* Dibuja una linea de texto con syntax highlighting token por token */
static void
render_highlighted(SDL_Renderer *renderer, TTF_Font *fuente,
                   const char *text, int x, int y)
{
    int p   = 0;
    int len = (int)strlen(text);

    while (p < len) {
        unsigned char c = (unsigned char)text[p];

        /* Espacios: avanzar x sin dibujar */
        if (c == ' ') {
            int sw; TTF_SizeUTF8(fuente, " ", &sw, NULL);
            x += sw;
            p++;
            continue;
        }

        char     tok[512];
        int      tok_len = 0;
        SDL_Color color;

        /* Comentario: // hasta el final */
        if (c == '/' && p+1 < len && text[p+1] == '/') {
            strncpy(tok, text + p, sizeof(tok) - 1);
            tok[sizeof(tok) - 1] = '\0';
            tok_len = len - p;
            color   = (SDL_Color){C_COMMENT};
        }
        /* String: "..."  (34 = ASCII double quote) */
        else if (c == 34) {
            int e = p + 1;
            while (e < len && (unsigned char)text[e] != 34) e++;
            if (e < len) e++;   /* incluir la comilla de cierre */
            tok_len = e - p;
            strncpy(tok, text + p, tok_len);
            tok[tok_len] = '\0';
            color = (SDL_Color){C_STRING};
        }
        /* Numero: empieza con digito */
        else if (c >= '0' && c <= '9') {
            int e = p;
            while (e < len) {
                unsigned char ec = (unsigned char)text[e];
                if ((ec >= '0' && ec <= '9') || ec == '.') e++;
                else break;
            }
            tok_len = e - p;
            strncpy(tok, text + p, tok_len);
            tok[tok_len] = '\0';
            color = (SDL_Color){C_NUMBER};
        }
        /* Palabra: letras, digitos, guion_bajo */
        else if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            int e = p;
            while (e < len) {
                unsigned char ec = (unsigned char)text[e];
                if ((ec >= 'a' && ec <= 'z') || (ec >= 'A' && ec <= 'Z') ||
                    (ec >= '0' && ec <= '9') || ec == '_') e++;
                else break;
            }
            tok_len = e - p;
            strncpy(tok, text + p, tok_len);
            tok[tok_len] = '\0';
            color = get_token_color(tok);
        }
        /* Resto: :=  ;  (  )  etc. */
        else {
            tok[0] = (char)c; tok[1] = '\0';
            tok_len = 1;
            color = (SDL_Color){C_NORMAL};
        }

        /* Renderizar token */
        SDL_Surface *s = TTF_RenderUTF8_Blended(fuente, tok, color);
        if (s) {
            SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
            SDL_Rect dst = { x, y, s->w, s->h };
            SDL_RenderCopy(renderer, t, NULL, &dst);
            x += s->w;
            SDL_FreeSurface(s);
            SDL_DestroyTexture(t);
        }

        p += tok_len;
    }
}

/* ── helpers de archivo ───────────────────────────────────────────── */
#ifndef _WIN32
#include <unistd.h>
#include <dirent.h>

/* Escanea saves/ y llena lista[] con nombres sin extension .paed */
static int
escanear_saves(char lista[][64], int max)
{
    int n = 0;
    DIR *d = opendir("saves");
    if (!d) return 0;
    struct dirent *e;
    while (n < max && (e = readdir(d))) {
        int l = (int)strlen(e->d_name);
        if (l > 5 && strcmp(e->d_name + l - 5, ".paed") == 0) {
            int nl = l - 5;
            if (nl >= (int)sizeof(lista[0])) nl = (int)sizeof(lista[0]) - 1;
            strncpy(lista[n], e->d_name, nl);
            lista[n][nl] = '\0';
            n++;
        }
    }
    closedir(d);
    /* orden alfabetico simple (bubble) */
    for (int i = 0; i < n-1; i++)
        for (int j = i+1; j < n; j++)
            if (strcmp(lista[i], lista[j]) > 0) {
                char tmp[64]; strncpy(tmp, lista[i], 63);
                strncpy(lista[i], lista[j], 63); strncpy(lista[j], tmp, 63);
            }
    return n;
}

/* Guarda el buffer completo en `path` */
static void
guardar_archivo(char buf[][512], int n, const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < n; i++) fprintf(f, "%s\n", buf[i]);
    fclose(f);
}

/* Ejecuta paed sobre `path` y llena out[][] con el output */
static int
ejecutar_paed(const char *path, char out[][256], int max)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "cd Frankly && ./paed '../%s' 2>&1", path);
    FILE *fp = popen(cmd, "r");
    if (!fp) { strncpy(out[0], "error: no se pudo ejecutar paed", 255); return 1; }
    int  n = 0;
    char lb[512];
    while (n < max && fgets(lb, sizeof(lb), fp)) {
        int l = (int)strlen(lb);
        if (l > 0 && lb[l-1] == '\n') lb[l-1] = '\0';
        strncpy(out[n], lb, 255); out[n][255] = '\0'; n++;
    }
    pclose(fp);
    return n > 0 ? n : 0;
}
#endif

/* ── screenEditorText ─────────────────────────────────────────────────
 * Editor multi-linea completo. ESC/F10 para salir.
 * ─────────────────────────────────────────────────────────────────── */
#define MAX_LINES 500

int
screenEditorText(SDL_Renderer *renderer, TTF_Font *fuente,
                 int ancho, int alto, const char *nombre_fijo,
                 const char *cons_titulo, const char *cons_texto)
{
    /* ── Buffer de lineas ─────────────────────────────────────────── */
    static char buf[MAX_LINES][512];
    int  n_lines    = 1;
    int  cursor_row = 0;
    int  cursor_col = 0;
    int  offset_row = 0;   /* primera linea visible (scroll) */
    memset(buf, 0, sizeof(buf));

    /* ── Layout ───────────────────────────────────────────────────── */
    int tiene_consigna = (cons_titulo && cons_titulo[0] != '\0');
    int editor_x  = tiene_consigna ? ancho / 2 : 0;
    int editor_w  = tiene_consigna ? ancho / 2 : ancho;
    int line_h    = TTF_FontHeight(fuente) + 4;
    int out_h     = line_h * 6 + 24;
    int edit_h    = alto - out_h;
    int text_x    = editor_x + GUTTER_W + 12;
    int vis_lines = (edit_h - 8) / line_h;

    /* ── Guardar ──────────────────────────────────────────────────── */
    int tiene_nombre_fijo = (nombre_fijo && nombre_fijo[0] != '\0');
    char nombre_arch[64];
    if (tiene_nombre_fijo) strncpy(nombre_arch, nombre_fijo, sizeof(nombre_arch)-1);
    else                   nombre_arch[0] = '\0';
    nombre_arch[sizeof(nombre_arch)-1] = '\0';

    char prompt_buf[64]  = "";
    int  prompt_len      = 0;
    int  guardando       = 0;
    int  dirty           = 0;   /* 1 = hay cambios sin guardar */
    int  confirm_salir   = 0;   /* 1 = mostrando dialogo "salir sin guardar?" */

#ifndef _WIN32
    /* ── Lista de archivos guardados (para overlay F9) ────────────── */
#define MAX_SAVES 50
    char saves_lista[MAX_SAVES][64];
    int  saves_n        = 0;
    int  saves_sel      = -1;   /* fila seleccionada con flechas */
    int  saves_offset   = 0;    /* scroll de la lista */
#endif

    /* ── Cargar archivo existente (solo si tiene nombre fijo) ─────── */
#ifndef _WIN32
    if (tiene_nombre_fijo) {
        char path[128];
        snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
        FILE *f = fopen(path, "r");
        if (f) {
            n_lines = 0;
            char linea[512];
            while (n_lines < MAX_LINES && fgets(linea, sizeof(linea), f)) {
                int l = (int)strlen(linea);
                if (l > 0 && linea[l-1] == '\n') linea[l-1] = '\0';
                strncpy(buf[n_lines], linea, 511);
                buf[n_lines][511] = '\0';
                n_lines++;
            }
            fclose(f);
            if (n_lines == 0) n_lines = 1;
        }
    }
#endif

    /* ── Salida ───────────────────────────────────────────────────── */
#define OUT_MAX 6
    char out_buf[OUT_MAX][256];
    int  n_out = 0;
    memset(out_buf, 0, sizeof(out_buf));

    SDL_Color c_num    = { LN_R, 255 };
    SDL_Color c_ph     = { PH_R, 255 };
    SDL_Color c_out    = {   0, 210,  75, 255 };
    SDL_Color c_out_hd = {   0, 155,  50, 255 };
    SDL_Color c_hint   = {  50,  90,  60, 255 };

    SDL_StartTextInput();
    SDL_Event evento;
    int corriendo = 1;

    while (corriendo) {

        /* ── Eventos ──────────────────────────────────────────────── */
        while (SDL_PollEvent(&evento)) {
            if (evento.type == SDL_QUIT) { SDL_StopTextInput(); return 0; }

            if (evento.type == SDL_KEYDOWN) {
                SDL_Keycode k = evento.key.keysym.sym;

                /* ── Dialogo confirmar salida ──────────────────────── */
                if (confirm_salir) {
                    if (k == SDLK_s || k == SDLK_RETURN) { corriendo = 0; }
                    if (k == SDLK_n || k == SDLK_ESCAPE)  confirm_salir = 0;
                    break;
                }

                /* ── Modo guardar nombre ───────────────────────────── */
                if (guardando) {
                    if (k == SDLK_ESCAPE) {
                        guardando = 0; prompt_len = 0; prompt_buf[0] = '\0';
                    }
                    if (k == SDLK_BACKSPACE && prompt_len > 0) {
                        prompt_buf[--prompt_len] = '\0';
#ifndef _WIN32
                        saves_sel = -1; /* al editar texto, deseleccionar lista */
#endif
                    }
#ifndef _WIN32
                    /* Navegar lista con flechas */
                    if (k == SDLK_UP) {
                        if (saves_sel > 0) saves_sel--;
                        else saves_sel = saves_n - 1;
                        if (saves_sel >= 0) {
                            strncpy(prompt_buf, saves_lista[saves_sel], sizeof(prompt_buf)-1);
                            prompt_len = (int)strlen(prompt_buf);
                        }
                    }
                    if (k == SDLK_DOWN) {
                        if (saves_sel < saves_n - 1) saves_sel++;
                        else saves_sel = 0;
                        if (saves_sel >= 0) {
                            strncpy(prompt_buf, saves_lista[saves_sel], sizeof(prompt_buf)-1);
                            prompt_len = (int)strlen(prompt_buf);
                        }
                    }
                    /* scroll: mantener seleccion visible (vis = 8 filas) */
                    if (saves_sel >= 0) {
                        if (saves_sel < saves_offset) saves_offset = saves_sel;
                        if (saves_sel >= saves_offset + 8) saves_offset = saves_sel - 7;
                    }
                    /* Delete: borrar archivo seleccionado */
                    if (k == SDLK_DELETE && saves_sel >= 0) {
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", saves_lista[saves_sel]);
                        remove(path);
                        saves_n   = escanear_saves(saves_lista, MAX_SAVES);
                        if (saves_sel >= saves_n) saves_sel = saves_n - 1;
                        if (saves_sel >= 0) {
                            strncpy(prompt_buf, saves_lista[saves_sel], sizeof(prompt_buf)-1);
                            prompt_len = (int)strlen(prompt_buf);
                        } else { prompt_buf[0] = '\0'; prompt_len = 0; }
                    }
#endif
                    if (k == SDLK_RETURN && prompt_len > 0) {
                        strncpy(nombre_arch, prompt_buf, sizeof(nombre_arch)-1);
                        nombre_arch[sizeof(nombre_arch)-1] = '\0';
#ifndef _WIN32
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                        guardar_archivo(buf, n_lines, path);
#endif
                        dirty     = 0;
                        guardando = 0; prompt_len = 0; prompt_buf[0] = '\0';
                    }
                    break;
                }

                /* ── Editor normal ─────────────────────────────────── */
                if (k == SDLK_F10) {
                    if (dirty) confirm_salir = 1;
                    else       corriendo = 0;
                    break;
                }
                if (k == SDLK_F8)  { screenDoc(renderer, fuente, ancho, alto); break; }

                /* ENTER: partir linea en cursor_col */
                if (k == SDLK_RETURN && n_lines < MAX_LINES) {
                    char resto[512];
                    strncpy(resto, buf[cursor_row] + cursor_col, 511);
                    resto[511] = '\0';
                    buf[cursor_row][cursor_col] = '\0';
                    for (int i = n_lines; i > cursor_row + 1; i--)
                        memcpy(buf[i], buf[i-1], 512);
                    strncpy(buf[cursor_row + 1], resto, 511);
                    buf[cursor_row + 1][511] = '\0';
                    n_lines++;
                    cursor_row++;
                    cursor_col = 0;
                    dirty = 1;
                }

                /* BACKSPACE */
                if (k == SDLK_BACKSPACE) {
                    if (cursor_col > 0) {
                        int len = (int)strlen(buf[cursor_row]);
                        memmove(buf[cursor_row] + cursor_col - 1,
                                buf[cursor_row] + cursor_col,
                                len - cursor_col + 1);
                        cursor_col--;
                        dirty = 1;
                    } else if (cursor_row > 0) {
                        /* al inicio: merge con linea anterior */
                        int prev_len = (int)strlen(buf[cursor_row - 1]);
                        int cur_len  = (int)strlen(buf[cursor_row]);
                        if (prev_len + cur_len < 511) {
                            strcat(buf[cursor_row - 1], buf[cursor_row]);
                            for (int i = cursor_row; i < n_lines - 1; i++)
                                memcpy(buf[i], buf[i+1], 512);
                            buf[n_lines - 1][0] = '\0';
                            n_lines--;
                            cursor_row--;
                            cursor_col = prev_len;
                            dirty = 1;
                        }
                    }
                }

                /* Flechas */
                if (k == SDLK_LEFT) {
                    if (cursor_col > 0) cursor_col--;
                    else if (cursor_row > 0) {
                        cursor_row--;
                        cursor_col = (int)strlen(buf[cursor_row]);
                    }
                }
                if (k == SDLK_RIGHT) {
                    int cur_len = (int)strlen(buf[cursor_row]);
                    if (cursor_col < cur_len) cursor_col++;
                    else if (cursor_row < n_lines - 1) { cursor_row++; cursor_col = 0; }
                }
                if (k == SDLK_UP && cursor_row > 0) {
                    cursor_row--;
                    int cur_len = (int)strlen(buf[cursor_row]);
                    if (cursor_col > cur_len) cursor_col = cur_len;
                }
                if (k == SDLK_DOWN && cursor_row < n_lines - 1) {
                    cursor_row++;
                    int cur_len = (int)strlen(buf[cursor_row]);
                    if (cursor_col > cur_len) cursor_col = cur_len;
                }
                if (k == SDLK_HOME) cursor_col = 0;
                if (k == SDLK_END)  cursor_col = (int)strlen(buf[cursor_row]);

                /* Scroll: mantener cursor visible */
                if (cursor_row < offset_row) offset_row = cursor_row;
                if (cursor_row >= offset_row + vis_lines)
                    offset_row = cursor_row - vis_lines + 1;

                /* F9: guardar */
                if (k == SDLK_F9) {
                    if (tiene_nombre_fijo) {
                        /* nombre fijo: sobreescribir directo */
#ifndef _WIN32
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                        guardar_archivo(buf, n_lines, path);
#endif
                        dirty = 0;
                    } else {
                        /* nombre libre: abrir pantalla de guardado */
                        guardando  = 1;
                        if (nombre_arch[0] != '\0')
                            strncpy(prompt_buf, nombre_arch, sizeof(prompt_buf)-1);
                        else
                            prompt_buf[0] = '\0';
                        prompt_len = (int)strlen(prompt_buf);
#ifndef _WIN32
                        saves_n   = escanear_saves(saves_lista, MAX_SAVES);
                        saves_sel = -1; saves_offset = 0;
#endif
                    }
                }

#ifndef _WIN32
                /* F5: guardar y ejecutar */
                if (k == SDLK_F5) {
                    char path[128];
                    snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                    guardar_archivo(buf, n_lines, path);
                    n_out = ejecutar_paed(path, out_buf, OUT_MAX);
                }
#endif
            }

            /* Click en [X] de la lista de guardado */
#ifndef _WIN32
            if (evento.type == SDL_MOUSEBUTTONDOWN && guardando &&
                evento.button.button == SDL_BUTTON_LEFT) {
                int mx = evento.button.x, my = evento.button.y;
                /* recalcular layout del overlay (igual que en render) */
                int bw2 = editor_w - 40, bx2 = editor_x + 20, by2 = 40;
                int iy2 = by2 + line_h + 22;
                int ly2 = iy2 + line_h + 10 + line_h + 4 + 4;
                int vis2 = (by2 + (alto-80) - ly2 - line_h - 20) / (line_h + 2);
                int x_btn = bx2 + bw2 - 36; /* posicion X del boton [X] */
                for (int i = 0; i < vis2 && (saves_offset + i) < saves_n; i++) {
                    int idx = saves_offset + i;
                    int fy  = ly2 + i * (line_h + 2);
                    SDL_Rect btn = { x_btn, fy - 2, 28, line_h + 4 };
                    if (mx >= btn.x && mx <= btn.x + btn.w &&
                        my >= btn.y && my <= btn.y + btn.h) {
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", saves_lista[idx]);
                        remove(path);
                        saves_n = escanear_saves(saves_lista, MAX_SAVES);
                        if (saves_sel >= saves_n) saves_sel = saves_n - 1;
                        if (saves_sel >= 0) {
                            strncpy(prompt_buf, saves_lista[saves_sel], sizeof(prompt_buf)-1);
                            prompt_len = (int)strlen(prompt_buf);
                        } else { prompt_buf[0] = '\0'; prompt_len = 0; }
                        break;
                    }
                }
            }
#endif

            if (evento.type == SDL_TEXTINPUT) {
                if (guardando) {
                    int add = (int)strlen(evento.text.text);
                    if (prompt_len + add < (int)sizeof(prompt_buf) - 1) {
                        strcat(prompt_buf, evento.text.text);
                        prompt_len += add;
                    }
                } else {
                    int add     = (int)strlen(evento.text.text);
                    int cur_len = (int)strlen(buf[cursor_row]);
                    if (cur_len + add < 511) {
                        memmove(buf[cursor_row] + cursor_col + add,
                                buf[cursor_row] + cursor_col,
                                cur_len - cursor_col + 1);
                        memcpy(buf[cursor_row] + cursor_col, evento.text.text, add);
                        cursor_col += add;
                        dirty = 1;
                    }
                }
            }
        }

        /* ── Render ───────────────────────────────────────────────── */
        SDL_SetRenderDrawColor(renderer, BG_R, 255);
        SDL_RenderClear(renderer);

        /* Panel consigna (izquierda) */
        if (tiene_consigna) {
            SDL_Rect panel_izq = { 0, 0, editor_x, alto };
            SDL_SetRenderDrawColor(renderer, 10, 18, 10, 255);
            SDL_RenderFillRect(renderer, &panel_izq);
            SDL_Color c_tit = { 0, 230, 80, 255 };
            SDL_Color c_txt = { 0, 190, 70, 255 };
            dibujadoTextoColor(renderer, fuente, cons_titulo, 14, 12, c_tit);
            SDL_SetRenderDrawColor(renderer, 0, 130, 50, 255);
            SDL_RenderDrawLine(renderer, 14, 46, editor_x - 14, 46);
            dibujadoTextoMultilineaColor(renderer, fuente, cons_texto,
                                         14, 56, editor_x - 28, c_txt);
            /* separador vertical */
            SDL_SetRenderDrawColor(renderer, 0, 100, 40, 255);
            SDL_RenderDrawLine(renderer, editor_x, 0, editor_x, alto);
        }

        /* Fondo editor */
        SDL_Rect editor_bg = { editor_x, 0, editor_w, edit_h };
        SDL_SetRenderDrawColor(renderer, BG_R, 255);
        SDL_RenderFillRect(renderer, &editor_bg);

        /* Gutter */
        SDL_Rect gutter = { editor_x, 0, GUTTER_W, edit_h };
        SDL_SetRenderDrawColor(renderer, GT_R, 255);
        SDL_RenderFillRect(renderer, &gutter);
        SDL_SetRenderDrawColor(renderer, LN_R, 255);
        SDL_RenderDrawLine(renderer, editor_x + GUTTER_W, 0, editor_x + GUTTER_W, edit_h);

        /* Lineas visibles */
        SDL_Rect edit_clip = { editor_x, 0, editor_w, edit_h };
        SDL_RenderSetClipRect(renderer, &edit_clip);

        for (int i = 0; i < vis_lines; i++) {
            int row = offset_row + i;
            if (row >= n_lines) break;
            int y = 8 + i * line_h;

            /* Numero de linea alineado a la derecha del gutter */
            char num[8];
            snprintf(num, sizeof(num), "%d", row + 1);
            int nw; TTF_SizeUTF8(fuente, num, &nw, NULL);
            SDL_Surface *sn = TTF_RenderUTF8_Blended(fuente, num, c_num);
            if (sn) {
                SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, sn);
                SDL_Rect pos = { editor_x + GUTTER_W - nw - 8, y, sn->w, sn->h };
                SDL_RenderCopy(renderer, t, NULL, &pos);
                SDL_FreeSurface(sn); SDL_DestroyTexture(t);
            }

            /* Texto de la linea (o placeholder en linea 1 vacia) */
            if (buf[row][0] == '\0' && n_lines == 1) {
                SDL_Surface *s = TTF_RenderUTF8_Blended(fuente,
                                     "Escribe tu codigo aqui...", c_ph);
                if (s) {
                    SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
                    SDL_Rect pos = { text_x, y, s->w, s->h };
                    SDL_RenderCopy(renderer, t, NULL, &pos);
                    SDL_FreeSurface(s); SDL_DestroyTexture(t);
                }
            } else if (buf[row][0] != '\0') {
                render_highlighted(renderer, fuente, buf[row], text_x, y);
            }

            /* Cursor fijo en la fila activa */
            if (row == cursor_row) {
                int cx = text_x;
                if (cursor_col > 0) {
                    char antes[512];
                    strncpy(antes, buf[cursor_row], cursor_col);
                    antes[cursor_col] = '\0';
                    int tw; TTF_SizeUTF8(fuente, antes, &tw, NULL);
                    cx = text_x + tw;
                }
                SDL_SetRenderDrawColor(renderer, TX_R, 200);
                SDL_Rect cur = { cx, y, 2, line_h - 2 };
                SDL_RenderFillRect(renderer, &cur);
            }
        }
        SDL_RenderSetClipRect(renderer, NULL);

        /* Nombre de archivo (arriba a la derecha) */
        {
            char titulo[80];
            if (nombre_arch[0] != '\0') snprintf(titulo, sizeof(titulo), "%s.paed", nombre_arch);
            else                        snprintf(titulo, sizeof(titulo), "sin guardar");
            SDL_Color c_arch = { LN_R, 255 };
            int tw; TTF_SizeUTF8(fuente, titulo, &tw, NULL);
            dibujadoTextoSimple(renderer, fuente, titulo, editor_x + editor_w - tw - 12, 4, c_arch);
        }

        /* ── Panel de salida ─────────────────────────────────────── */
        SDL_SetRenderDrawColor(renderer, 0, 170, 55, 255);
        SDL_RenderDrawLine(renderer, editor_x, edit_h, editor_x + editor_w, edit_h);
        SDL_SetRenderDrawColor(renderer, 0, 70, 22, 255);
        SDL_RenderDrawLine(renderer, editor_x, edit_h + 1, editor_x + editor_w, edit_h + 1);

        SDL_Rect out_area = { editor_x, edit_h + 2, editor_w, out_h - 2 };
        SDL_SetRenderDrawColor(renderer, 6, 12, 6, 255);
        SDL_RenderFillRect(renderer, &out_area);

        SDL_Rect hd = { editor_x, edit_h + 2, editor_w, line_h + 4 };
        SDL_SetRenderDrawColor(renderer, 0, 38, 13, 255);
        SDL_RenderFillRect(renderer, &hd);
        dibujadoTextoSimple(renderer, fuente, "SALIDA", editor_x + 10, edit_h + 4, c_out_hd);
        dibujadoTextoSimple(renderer, fuente,
            "[F5] ejecutar   [F8] doc   [F9] guardar   [F10] volver",
            editor_x + editor_w - 480, edit_h + 4, c_hint);

        SDL_SetRenderDrawColor(renderer, 0, 55, 18, 160);
        SDL_RenderDrawLine(renderer, editor_x, edit_h + line_h + 8, editor_x + editor_w, edit_h + line_h + 8);

        SDL_RenderSetClipRect(renderer, &out_area);
        int oy = edit_h + line_h + 12;
        for (int i = 0; i < n_out; i++) {
            if (oy + line_h > alto) break;
            char ms[300]; snprintf(ms, sizeof(ms), "> %s", out_buf[i]);
            dibujadoTextoSimple(renderer, fuente, ms, editor_x + 10, oy, c_out);
            oy += line_h + 2;
        }
        SDL_RenderSetClipRect(renderer, NULL);

        /* ── Pantalla de guardado (overlay) ─────────────────────────── */
        if (guardando) {
            /* oscurecer fondo */
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_Rect sombra2 = { 0, 0, ancho, alto };
            SDL_RenderFillRect(renderer, &sombra2);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            /* caja centrada */
            int bw = editor_w - 40;  int bh = alto - 80;
            int bx = editor_x + 20;  int by = 40;
            SDL_Rect box = { bx, by, bw, bh };
            SDL_SetRenderDrawColor(renderer, 20, 22, 20, 255);
            SDL_RenderFillRect(renderer, &box);
            SDL_SetRenderDrawColor(renderer, 0, 170, 55, 255);
            SDL_RenderDrawRect(renderer, &box);

            SDL_Color c_hdr  = {   0, 210,  75, 255 };
            SDL_Color c_lbl  = { 146, 131, 116, 255 };
            SDL_Color c_txt  = { 235, 219, 178, 255 };
            SDL_Color c_sel  = {   0,  80,  30, 255 };
            SDL_Color c_item = { 142, 192, 124, 255 };
            SDL_Color c_hint2= {  80,  73,  69, 255 };

            /* cabecera */
            dibujadoTextoSimple(renderer, fuente, "Guardar archivo",
                                bx + 14, by + 10, c_hdr);
            SDL_SetRenderDrawColor(renderer, 0, 120, 40, 255);
            SDL_RenderDrawLine(renderer, bx + 10, by + line_h + 14,
                                          bx + bw - 10, by + line_h + 14);

            /* campo de nombre (input) */
            int iy = by + line_h + 22;
            dibujadoTextoSimple(renderer, fuente, "Nombre:", bx + 14, iy, c_lbl);
            int lw; TTF_SizeUTF8(fuente, "Nombre: ", &lw, NULL);

            SDL_Rect input_bg = { bx + 12, iy - 2, bw - 24, line_h + 4 };
            SDL_SetRenderDrawColor(renderer, 30, 40, 30, 255);
            SDL_RenderFillRect(renderer, &input_bg);
            SDL_SetRenderDrawColor(renderer, 0, 130, 50, 255);
            SDL_RenderDrawRect(renderer, &input_bg);

            dibujadoTextoSimple(renderer, fuente, prompt_buf,
                                bx + 14 + lw, iy, c_txt);
            /* cursor de texto */
            {
                int tw = 0;
                if (prompt_len > 0) TTF_SizeUTF8(fuente, prompt_buf, &tw, NULL);
                SDL_SetRenderDrawColor(renderer, 235, 219, 178, 200);
                SDL_Rect pc = { bx + 14 + lw + tw, iy, 2, line_h - 2 };
                SDL_RenderFillRect(renderer, &pc);
            }

            /* separador lista */
            int ly = iy + line_h + 10;
            dibujadoTextoSimple(renderer, fuente, "Archivos guardados:",
                                bx + 14, ly, c_lbl);
            ly += line_h + 4;
            SDL_SetRenderDrawColor(renderer, 0, 80, 25, 255);
            SDL_RenderDrawLine(renderer, bx + 10, ly, bx + bw - 10, ly);
            ly += 4;

#ifndef _WIN32
            int vis_saves = (by + bh - ly - line_h - 20) / (line_h + 2);
            if (vis_saves < 1) vis_saves = 1;

            if (saves_n == 0) {
                dibujadoTextoSimple(renderer, fuente, "(no hay archivos guardados)",
                                    bx + 20, ly, c_hint2);
            }
            for (int i = 0; i < vis_saves && (saves_offset + i) < saves_n; i++) {
                int idx = saves_offset + i;
                int fy  = ly + i * (line_h + 2);
                if (idx == saves_sel) {
                    SDL_Rect sel_bg = { bx + 10, fy - 2, bw - 20, line_h + 4 };
                    SDL_SetRenderDrawColor(renderer, c_sel.r, c_sel.g, c_sel.b, 255);
                    SDL_RenderFillRect(renderer, &sel_bg);
                }
                char entry[80];
                snprintf(entry, sizeof(entry), "%s.paed", saves_lista[idx]);
                SDL_Color ce = (idx == saves_sel) ? c_txt : c_item;
                dibujadoTextoSimple(renderer, fuente, entry, bx + 20, fy, ce);

                /* boton [X] rojo a la derecha */
                SDL_Rect btn_x = { bx + bw - 36, fy - 2, 28, line_h + 4 };
                SDL_SetRenderDrawColor(renderer, 100, 15, 15, 255);
                SDL_RenderFillRect(renderer, &btn_x);
                SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
                SDL_RenderDrawRect(renderer, &btn_x);
                SDL_Color c_x = { 251, 73, 52, 255 };
                dibujadoTextoSimple(renderer, fuente, "X",
                                    btn_x.x + 8, fy, c_x);
            }
#endif

            /* hint inferior */
            dibujadoTextoSimple(renderer, fuente,
                "[↑↓] seleccionar   [Enter] guardar   [ESC] cancelar",
                bx + 14, by + bh - line_h - 8, c_hint2);
        }

        /* ── Dialogo "salir sin guardar?" (overlay) ─────────────── */
        if (confirm_salir) {
            /* oscurecer fondo */
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
            SDL_Rect sombra = { 0, 0, ancho, alto };
            SDL_RenderFillRect(renderer, &sombra);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

            /* caja centrada */
            int bw = 380, bh = 80;
            int bx = editor_x + (editor_w - bw) / 2, by = (alto - bh) / 2;
            SDL_Rect box = { bx, by, bw, bh };
            SDL_SetRenderDrawColor(renderer, 28, 24, 22, 255);
            SDL_RenderFillRect(renderer, &box);
            SDL_SetRenderDrawColor(renderer, 251, 73, 52, 255);   /* rojo gruvbox */
            SDL_RenderDrawRect(renderer, &box);

            SDL_Color c_pregunta = { 235, 219, 178, 255 };
            SDL_Color c_opciones = { 146, 131, 116, 255 };
            dibujadoTextoSimple(renderer, fuente,
                "Salir sin guardar?",
                bx + 20, by + 12, c_pregunta);
            dibujadoTextoSimple(renderer, fuente,
                "[S / Enter] Si     [N / ESC] No",
                bx + 20, by + 12 + line_h + 4, c_opciones);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    return 0;
}
