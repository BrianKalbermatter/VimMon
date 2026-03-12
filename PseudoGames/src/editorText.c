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

/* ── Clipboard multi-linea ────────────────────────────────────────── */
#define MAX_LINES 500
#define CB_MAX 50
static char cb_buf[CB_MAX][512];
static int  cb_n = 0;

/* Copia lineas [r0..r1] del buffer en cb_buf y en el portapapeles del sistema */
static void
editor_copiar(char buf[][512], int r0, int r1)
{
    if (r0 > r1) { int t = r0; r0 = r1; r1 = t; }
    cb_n = 0;
    char texto[CB_MAX * 513];
    texto[0] = '\0';
    for (int i = r0; i <= r1 && cb_n < CB_MAX; i++, cb_n++) {
        strncpy(cb_buf[cb_n], buf[i], 511);
        cb_buf[cb_n][511] = '\0';
        strncat(texto, cb_buf[cb_n], sizeof(texto) - strlen(texto) - 2);
        if (i < r1) strncat(texto, "\n", sizeof(texto) - strlen(texto) - 1);
    }
    SDL_SetClipboardText(texto);
}

/* Corta lineas [r0..r1]: las copia y las elimina del buffer */
static void
editor_cortar(char buf[][512], int *n_lines, int r0, int r1,
              int *cursor_row, int *cursor_col)
{
    if (r0 > r1) { int t = r0; r0 = r1; r1 = t; }
    editor_copiar(buf, r0, r1);
    int count = r1 - r0 + 1;
    for (int i = r0; i + count < *n_lines; i++)
        memcpy(buf[i], buf[i + count], 512);
    for (int i = *n_lines - count; i < *n_lines; i++)
        buf[i][0] = '\0';
    *n_lines -= count;
    if (*n_lines < 1) { *n_lines = 1; buf[0][0] = '\0'; }
    if (*cursor_row >= *n_lines) *cursor_row = *n_lines - 1;
    *cursor_col = 0;
}

/* Pega cb_buf (o el portapapeles del sistema) despues de cursor_row */
static void
editor_pegar(char buf[][512], int *n_lines, int *cursor_row, int *cursor_col)
{
    /* Intentar leer desde portapapeles del sistema primero */
    char *sys = SDL_GetClipboardText();
    if (sys && sys[0] != '\0') {
        static char tmp[CB_MAX][512];
        int tn = 0;
        char *p = sys;
        while (*p && tn < CB_MAX) {
            char *nl = strchr(p, '\n');
            int len = nl ? (int)(nl - p) : (int)strlen(p);
            if (len > 511) len = 511;
            strncpy(tmp[tn], p, len);
            tmp[tn][len] = '\0';
            tn++;
            if (!nl) break;
            p = nl + 1;
        }
        SDL_free(sys);
        if (tn > 0) {
            cb_n = tn;
            for (int i = 0; i < tn; i++) strncpy(cb_buf[i], tmp[i], 511);
        }
    } else {
        if (sys) SDL_free(sys);
    }

    if (cb_n == 0) return;
    if (*n_lines + cb_n > MAX_LINES) return;

    int ins = *cursor_row + 1;
    /* desplazar lineas existentes hacia abajo */
    for (int i = *n_lines - 1; i >= ins; i--)
        memcpy(buf[i + cb_n], buf[i], 512);
    /* insertar las lineas del clipboard */
    for (int i = 0; i < cb_n; i++)
        strncpy(buf[ins + i], cb_buf[i], 511);
    *n_lines  += cb_n;
    *cursor_row = ins + cb_n - 1;
    *cursor_col = (int)strlen(buf[*cursor_row]);
}

/* ── helpers de archivo ───────────────────────────────────────────── */
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
#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "cd Frankly && paed.exe \"../%s\" 2>&1", path);
#else
    snprintf(cmd, sizeof(cmd), "cd Frankly && ./paed '../%s' 2>&1", path);
#endif
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

/* ── screenEditorText ─────────────────────────────────────────────────
 * Editor multi-linea completo. ESC/F10 para salir.
 * ─────────────────────────────────────────────────────────────────── */

int
screenEditorText(SDL_Renderer *renderer, TTF_Font *fuente,
                 int ancho, int alto, const char *nombre_fijo,
                 const char *cons_titulo, const char *cons_texto)
{
    /* ── Buffer de lineas ─────────────────────────────────────────── */
    static char buf[MAX_LINES][512];
    static char libre_ultimo[64] = "";  /* ultimo archivo guardado en modo libre */
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

    /* Boton |> RUN (panel de salida, extremo derecho) */
    int btn_run_w = 72;
    SDL_Rect btn_run = { editor_x + editor_w - btn_run_w - 6, edit_h + 3, btn_run_w, line_h };

    /* Boton menu ≡ (a la izquierda del RUN) */
    int btn_menu_w = 30;
    SDL_Rect btn_menu = { btn_run.x - btn_menu_w - 6, edit_h + 3, btn_menu_w, line_h };

    /* Dropdown del mini menu */
    int   mini_menu   = 0;
    const char *menu_items[] = { "Volver al menu principal", "Guardar", "Cargar" };
    int   mitem_h   = line_h + 6;
    int   mdrop_w   = 260;
    int   mdrop_h   = mitem_h * 3 + 6;
    SDL_Rect mdrop  = { btn_menu.x + btn_menu_w - mdrop_w,
                        edit_h - mdrop_h - 4,
                        mdrop_w, mdrop_h };

    /* ── Guardar ──────────────────────────────────────────────────── */
    int tiene_nombre_fijo = (nombre_fijo && nombre_fijo[0] != '\0');
    char nombre_arch[64];
    if (tiene_nombre_fijo) strncpy(nombre_arch, nombre_fijo, sizeof(nombre_arch)-1);
    else                   nombre_arch[0] = '\0';
    nombre_arch[sizeof(nombre_arch)-1] = '\0';

    char prompt_buf[64]  = "";
    int  prompt_len      = 0;
    int  guardando       = 0;
    int  cargando        = 0;
    int  dirty           = 0;   /* 1 = hay cambios sin guardar */
    int  confirm_salir   = 0;   /* 1 = mostrando dialogo "salir sin guardar?" */

    /* ── Lista de archivos guardados (para overlay F9) ────────────── */
#define MAX_SAVES 50
    char saves_lista[MAX_SAVES][64];
    int  saves_n        = 0;
    int  saves_sel      = -1;   /* fila seleccionada con flechas */
    int  saves_offset   = 0;    /* scroll de la lista */

    /* ── Cargar archivo existente (solo si tiene nombre fijo) ─────── */
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

    /* ── Salida ───────────────────────────────────────────────────── */
#define OUT_MAX 6
    char out_buf[OUT_MAX][256];
    int  n_out = 0;
    memset(out_buf, 0, sizeof(out_buf));

    SDL_Color c_num    = { LN_R, 255 };
    SDL_Color c_ph     = { PH_R, 255 };
    SDL_Color c_out    = {   0, 210,  75, 255 };
    SDL_Color c_out_hd = {   0, 155,  50, 255 };


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
                        saves_sel = -1; /* al editar texto, deseleccionar lista */
                    }
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
                    if (k == SDLK_RETURN && prompt_len > 0) {
                        strncpy(nombre_arch, prompt_buf, sizeof(nombre_arch)-1);
                        nombre_arch[sizeof(nombre_arch)-1] = '\0';
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                        guardar_archivo(buf, n_lines, path);
                        dirty     = 0;
                        guardando = 0; prompt_len = 0; prompt_buf[0] = '\0';
                        /* recordar el nombre para auto-cargar la proxima vez */
                        if (!tiene_nombre_fijo)
                            strncpy(libre_ultimo, nombre_arch, sizeof(libre_ultimo)-1);
                    }
                    break;
                }

                /* ── Modo cargar archivo ───────────────────────────── */
                if (cargando) {
                    if (k == SDLK_ESCAPE) {
                        cargando = 0; prompt_len = 0; prompt_buf[0] = '\0';
                    }
                    if (k == SDLK_BACKSPACE && prompt_len > 0) {
                        prompt_buf[--prompt_len] = '\0';
                        saves_sel = -1;
                    }
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
                    if (saves_sel >= 0) {
                        if (saves_sel < saves_offset) saves_offset = saves_sel;
                        if (saves_sel >= saves_offset + 8) saves_offset = saves_sel - 7;
                    }
                    if (k == SDLK_DELETE && saves_sel >= 0) {
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", saves_lista[saves_sel]);
                        remove(path);
                        saves_n = escanear_saves(saves_lista, MAX_SAVES);
                        if (saves_sel >= saves_n) saves_sel = saves_n - 1;
                        if (saves_sel >= 0) {
                            strncpy(prompt_buf, saves_lista[saves_sel], sizeof(prompt_buf)-1);
                            prompt_len = (int)strlen(prompt_buf);
                        } else { prompt_buf[0] = '\0'; prompt_len = 0; }
                    }
                    if (k == SDLK_RETURN && prompt_len > 0) {
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", prompt_buf);
                        FILE *fl = fopen(path, "r");
                        if (fl) {
                            memset(buf, 0, sizeof(buf));
                            n_lines = 0;
                            char linea[512];
                            while (n_lines < MAX_LINES && fgets(linea, sizeof(linea), fl)) {
                                int l = (int)strlen(linea);
                                if (l > 0 && linea[l-1] == '\n') linea[l-1] = '\0';
                                strncpy(buf[n_lines], linea, 511);
                                buf[n_lines][511] = '\0';
                                n_lines++;
                            }
                            fclose(fl);
                            if (n_lines == 0) n_lines = 1;
                            strncpy(nombre_arch, prompt_buf, sizeof(nombre_arch)-1);
                            strncpy(libre_ultimo, prompt_buf, sizeof(libre_ultimo)-1);
                            cursor_row = 0; cursor_col = 0; offset_row = 0;
                            dirty = 0;
                        }
                        cargando = 0; prompt_len = 0; prompt_buf[0] = '\0';
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

                /* Ctrl+C: copiar linea actual */
                if (k == SDLK_c && (evento.key.keysym.mod & KMOD_CTRL))
                    editor_copiar(buf, cursor_row, cursor_row);

                /* Ctrl+X: cortar linea actual */
                if (k == SDLK_x && (evento.key.keysym.mod & KMOD_CTRL)) {
                    editor_cortar(buf, &n_lines, cursor_row, cursor_row,
                                  &cursor_row, &cursor_col);
                    dirty = 1;
                }

                /* Ctrl+V: pegar (una o multiples lineas) */
                if (k == SDLK_v && (evento.key.keysym.mod & KMOD_CTRL)) {
                    editor_pegar(buf, &n_lines, &cursor_row, &cursor_col);
                    dirty = 1;
                }

                /* Scroll: mantener cursor visible */
                if (cursor_row < offset_row) offset_row = cursor_row;
                if (cursor_row >= offset_row + vis_lines)
                    offset_row = cursor_row - vis_lines + 1;

                /* F9: guardar
                 * Si ya tiene nombre (fijo o dado antes): sobreescribe directo.
                 * Si no tiene nombre todavia: abre el overlay para elegir uno. */
                if (k == SDLK_F9) {
                    if (nombre_arch[0] != '\0') {
                        char path[128];
                        snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                        guardar_archivo(buf, n_lines, path);
                        dirty = 0;
                    } else {
                        /* sin nombre aun: abrir overlay */
                        guardando  = 1;
                        prompt_buf[0] = '\0';
                        prompt_len    = 0;
                        saves_n   = escanear_saves(saves_lista, MAX_SAVES);
                        saves_sel = -1; saves_offset = 0;
                    }
                }

                /* F5: guardar y ejecutar */
                if (k == SDLK_F5) {
                    char path[128];
                    snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                    guardar_archivo(buf, n_lines, path);
                    n_out = ejecutar_paed(path, out_buf, OUT_MAX);
                }
            }

            /* Click en [X] de la lista de guardado */
            if (evento.type == SDL_MOUSEBUTTONDOWN && (guardando || cargando) &&
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

            /* Click en boton |> ejecutar */
            if (evento.type == SDL_MOUSEBUTTONDOWN && !guardando && !cargando &&
                evento.button.button == SDL_BUTTON_LEFT) {
                int mx = evento.button.x, my = evento.button.y;
                if (mx >= btn_run.x && mx < btn_run.x + btn_run.w &&
                    my >= btn_run.y && my < btn_run.y + btn_run.h) {
                    char path[128];
                    snprintf(path, sizeof(path), "saves/%s.paed", nombre_arch);
                    guardar_archivo(buf, n_lines, path);
                    n_out = ejecutar_paed(path, out_buf, OUT_MAX);
                }
                /* Boton menu ≡: toggle dropdown */
                else if (mx >= btn_menu.x && mx < btn_menu.x + btn_menu.w &&
                         my >= btn_menu.y && my < btn_menu.y + btn_menu.h) {
                    mini_menu = !mini_menu;
                }
                /* Click dentro del dropdown: ejecutar item */
                else if (mini_menu &&
                         mx >= mdrop.x && mx < mdrop.x + mdrop.w &&
                         my >= mdrop.y && my < mdrop.y + mdrop.h) {
                    int item = (my - mdrop.y - 3) / mitem_h;
                    if (item == 0) {
                        /* Volver al menu principal: igual que F10 */
                        if (dirty) confirm_salir = 1;
                        else       corriendo = 0;
                    } else if (item == 1) {
                        /* Guardar: abre overlay con nombre actual pre-cargado.
                         * Siempre muestra el overlay para confirmar/cambiar nombre. */
                        guardando = 1;
                        if (nombre_arch[0] != '\0')
                            strncpy(prompt_buf, nombre_arch, sizeof(prompt_buf)-1);
                        else
                            prompt_buf[0] = '\0';
                        prompt_len = (int)strlen(prompt_buf);
                        saves_n    = escanear_saves(saves_lista, MAX_SAVES);
                        saves_sel  = -1; saves_offset = 0;
                    } else if (item == 2) {
                        /* Cargar: abrir overlay de seleccion */
                        cargando   = 1;
                        prompt_buf[0] = '\0';
                        prompt_len    = 0;
                        saves_n    = escanear_saves(saves_lista, MAX_SAVES);
                        saves_sel  = -1; saves_offset = 0;
                    }
                    mini_menu = 0;
                }
                /* Click fuera del dropdown: cerrar */
                else if (mini_menu) {
                    mini_menu = 0;
                }
            }

            if (evento.type == SDL_TEXTINPUT) {
                if (guardando || cargando) {
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
            char num[12];
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
        dibujadoTextoSimple(renderer, fuente, "SALIDA", editor_x + 4, edit_h + 4, c_out_hd);

        /* Boton menu ≡ */
        {
            int mx_h, my_h; SDL_GetMouseState(&mx_h, &my_h);
            int hov = (mx_h >= btn_menu.x && mx_h < btn_menu.x + btn_menu.w &&
                       my_h >= btn_menu.y && my_h < btn_menu.y + btn_menu.h);
            SDL_SetRenderDrawColor(renderer, hov || mini_menu ? 50 : 25,
                                             hov || mini_menu ? 80 : 45,
                                             hov || mini_menu ? 50 : 25, 255);
            SDL_RenderFillRect(renderer, &btn_menu);
            SDL_SetRenderDrawColor(renderer, 0, 160, 55, 255);
            SDL_RenderDrawRect(renderer, &btn_menu);
            int lw2; TTF_SizeUTF8(fuente, "=", &lw2, NULL);
            dibujadoTextoSimple(renderer, fuente, "=",
                                btn_menu.x + (btn_menu.w - lw2) / 2,
                                btn_menu.y, (SDL_Color){180, 200, 180, 255});
        }

        /* Boton |> RUN al extremo derecho */
        {
            int mx_h, my_h; SDL_GetMouseState(&mx_h, &my_h);
            int hover = (mx_h >= btn_run.x && mx_h < btn_run.x + btn_run.w &&
                         my_h >= btn_run.y && my_h < btn_run.y + btn_run.h);
            if (hover)
                SDL_SetRenderDrawColor(renderer, 60, 220, 90, 255);
            else
                SDL_SetRenderDrawColor(renderer, 20, 160, 55, 255);
            SDL_RenderFillRect(renderer, &btn_run);
            SDL_SetRenderDrawColor(renderer, 0, 230, 80, 255);
            SDL_RenderDrawRect(renderer, &btn_run);
            /* centrar texto "|> RUN" dentro del boton */
            int lbl_w; TTF_SizeUTF8(fuente, "|> RUN", &lbl_w, NULL);
            int lbl_x = btn_run.x + (btn_run.w - lbl_w) / 2;
            dibujadoTextoSimple(renderer, fuente, "|> RUN",
                                lbl_x, btn_run.y, (SDL_Color){10, 10, 10, 255});
        }

        /* Dropdown mini menu */
        if (mini_menu) {
            int mx_d, my_d; SDL_GetMouseState(&mx_d, &my_d);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 10, 18, 10, 240);
            SDL_RenderFillRect(renderer, &mdrop);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 180, 60, 255);
            SDL_RenderDrawRect(renderer, &mdrop);

            for (int i = 0; i < 3; i++) {
                int iy = mdrop.y + 3 + i * mitem_h;
                SDL_Rect item_r = { mdrop.x + 2, iy, mdrop.w - 4, mitem_h };
                int hov_i = (mx_d >= item_r.x && mx_d < item_r.x + item_r.w &&
                             my_d >= item_r.y && my_d < item_r.y + item_r.h);
                if (hov_i) {
                    SDL_SetRenderDrawColor(renderer, 20, 60, 25, 255);
                    SDL_RenderFillRect(renderer, &item_r);
                }
                /* separador entre items */
                if (i > 0) {
                    SDL_SetRenderDrawColor(renderer, 0, 70, 25, 255);
                    SDL_RenderDrawLine(renderer, mdrop.x + 6, iy - 1,
                                                mdrop.x + mdrop.w - 6, iy - 1);
                }
                SDL_Color c_mi = hov_i ? (SDL_Color){200, 240, 200, 255}
                                       : (SDL_Color){142, 192, 124, 255};
                dibujadoTextoSimple(renderer, fuente, menu_items[i],
                                    mdrop.x + 10, iy + 2, c_mi);
            }
        }

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

            /* hint inferior */
            dibujadoTextoSimple(renderer, fuente,
                "[↑↓] seleccionar   [Enter] guardar   [ESC] cancelar",
                bx + 14, by + bh - line_h - 8, c_hint2);
        }

        /* ── Pantalla de carga (overlay) ──────────────────────────── */
        if (cargando) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
            SDL_Rect sombra3 = { 0, 0, ancho, alto };
            SDL_RenderFillRect(renderer, &sombra3);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

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

            dibujadoTextoSimple(renderer, fuente, "Cargar archivo",
                                bx + 14, by + 10, c_hdr);
            SDL_SetRenderDrawColor(renderer, 0, 120, 40, 255);
            SDL_RenderDrawLine(renderer, bx + 10, by + line_h + 14,
                                          bx + bw - 10, by + line_h + 14);

            /* campo de nombre */
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
            {
                int tw = 0;
                if (prompt_len > 0) TTF_SizeUTF8(fuente, prompt_buf, &tw, NULL);
                SDL_SetRenderDrawColor(renderer, 235, 219, 178, 200);
                SDL_Rect pc = { bx + 14 + lw + tw, iy, 2, line_h - 2 };
                SDL_RenderFillRect(renderer, &pc);
            }

            /* lista de archivos */
            int ly = iy + line_h + 10;
            dibujadoTextoSimple(renderer, fuente, "Archivos guardados:",
                                bx + 14, ly, c_lbl);
            ly += line_h + 4;
            SDL_SetRenderDrawColor(renderer, 0, 80, 25, 255);
            SDL_RenderDrawLine(renderer, bx + 10, ly, bx + bw - 10, ly);
            ly += 4;

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

                /* boton [X] */
                SDL_Rect btn_x = { bx + bw - 36, fy - 2, 28, line_h + 4 };
                SDL_SetRenderDrawColor(renderer, 100, 15, 15, 255);
                SDL_RenderFillRect(renderer, &btn_x);
                SDL_SetRenderDrawColor(renderer, 200, 30, 30, 255);
                SDL_RenderDrawRect(renderer, &btn_x);
                SDL_Color c_x = { 251, 73, 52, 255 };
                dibujadoTextoSimple(renderer, fuente, "X", btn_x.x + 8, fy, c_x);
            }

            dibujadoTextoSimple(renderer, fuente,
                "[↑↓] seleccionar   [Enter] cargar   [ESC] cancelar",
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
