#include "parser.h"
#include "../../cjson/cJSON.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ── Fuente unica de verdad: sintaxis.json ─────────────────────────────────────

static cJSON *g_syntax = NULL;   // el lenguaje: pseudocodigo AED puro
static cJSON *g_escena = NULL;   // libreria opcional de VimMon, no es el lenguaje

static cJSON *cargar_json(const char *path, int obligatorio) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (obligatorio) fprintf(stderr, "[paed] no se pudo abrir %s\n", path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0) { fclose(f); return NULL; }

    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }

    size_t leidos = fread(buf, 1, (size_t)size, f);
    buf[leidos] = '\0';
    fclose(f);

    cJSON *raiz = cJSON_Parse(buf);
    free(buf);

    if (!raiz) fprintf(stderr, "[paed] %s tiene JSON invalido\n", path);
    return raiz;
}

int paed_syntax_load(void) {
    if (g_syntax) return 0;

    g_syntax = cargar_json(PAED_SYNTAX_PATH, 1);
    if (!g_syntax) return -1;

    // La libreria de escena es opcional: sin ella PAED sigue siendo AED puro.
    g_escena = cargar_json(PAED_ESCENA_PATH, 0);
    return 0;
}

void paed_syntax_free(void) {
    if (g_syntax) { cJSON_Delete(g_syntax); g_syntax = NULL; }
    if (g_escena) { cJSON_Delete(g_escena); g_escena = NULL; }
}

static cJSON *buscar_proc(cJSON *raiz, const char *nombre) {
    if (!raiz) return NULL;
    cJSON *p = NULL;
    cJSON_ArrayForEach(p, cJSON_GetObjectItem(raiz, "procedimientos")) {
        cJSON *n = cJSON_GetObjectItem(p, "nombre");
        if (cJSON_IsString(n) && strcmp(n->valuestring, nombre) == 0) return p;
    }
    return NULL;
}

// Primero el lenguaje, despues las librerias cargadas.
static cJSON *proc_def(const char *nombre) {
    cJSON *p = buscar_proc(g_syntax, nombre);
    return p ? p : buscar_proc(g_escena, nombre);
}

// Busca un parametro de un procedimiento por nombre o por alias.
static cJSON *param_def(cJSON *proc, const char *clave) {
    cJSON *params = cJSON_GetObjectItem(proc, "params");
    cJSON *p      = NULL;
    cJSON_ArrayForEach(p, params) {
        cJSON *n = cJSON_GetObjectItem(p, "nombre");
        cJSON *a = cJSON_GetObjectItem(p, "alias");
        if (cJSON_IsString(n) && strcmp(n->valuestring, clave) == 0) return p;
        if (cJSON_IsString(a) && strcmp(a->valuestring, clave) == 0) return p;
    }
    return NULL;
}

static int proc_es_variadico(cJSON *proc) {
    return cJSON_IsTrue(cJSON_GetObjectItem(proc, "variadico"));
}

// ── Utilidades de texto ───────────────────────────────────────────────────────

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) return s;
    char *fin = s + strlen(s) - 1;
    while (fin > s && isspace((unsigned char)*fin)) *fin-- = '\0';
    return s;
}

// Corta el comentario // (respetando comillas) y el salto de linea.
static void strip_comment(char *s) {
    int en_texto = 0;
    for (char *c = s; *c; c++) {
        if (*c == '"') en_texto = !en_texto;
        if (!en_texto && c[0] == '/' && c[1] == '/') { *c = '\0'; return; }
    }
}

static int es_identificador(const char *s) {
    if (!*s || (!isalpha((unsigned char)*s) && *s != '_')) return 0;
    for (const char *c = s; *c; c++)
        if (!isalnum((unsigned char)*c) && *c != '_') return 0;
    return 1;
}

// ── Errores ───────────────────────────────────────────────────────────────────

static void add_error(PAEDProgram *p, int line, const char *fmt, ...) {
    if (p->error_count >= PAED_MAX_ERRORS) return;
    PAEDError *e = &p->errors[p->error_count++];
    e->line = line;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(e->msg, sizeof(e->msg), fmt, ap);
    va_end(ap);
}

void paed_print_errors(const PAEDProgram *prog) {
    for (int i = 0; i < prog->error_count; i++)
        fprintf(stderr, "%s:%d: error: %s\n",
                prog->path, prog->errors[i].line, prog->errors[i].msg);
    if (prog->error_count >= PAED_MAX_ERRORS)
        fprintf(stderr, "%s: error: demasiados errores, se corto el reporte\n", prog->path);
}

// ── Validacion de valores segun el tipo declarado en sintaxis.json ────────────

static int valida_valor(const char *tipo, const char *val) {
    if (strcmp(tipo, "VEC3") == 0) {
        float x, y, z;
        return val[0] == '(' && sscanf(val, "(%f,%f,%f)", &x, &y, &z) == 3;
    }
    if (strcmp(tipo, "HEX") == 0) {
        if (val[0] != '#') return 0;
        size_t n = strlen(val + 1);
        if (n != 3 && n != 6 && n != 8) return 0;
        for (const char *c = val + 1; *c; c++)
            if (!isxdigit((unsigned char)*c)) return 0;
        return 1;
    }
    if (strcmp(tipo, "NUM") == 0) {
        char *fin = NULL;
        strtod(val, &fin);
        return fin && *fin == '\0' && fin != val;
    }
    if (strcmp(tipo, "ID") == 0) return es_identificador(val);
    return 1;  // tipo desconocido: no bloquea
}

// ── Parseo de una instruccion: PROC(clave = valor, ...); ──────────────────────

// Parte el interior de los parentesis en argumentos, cortando SOLO en las comas
// de nivel 0. Asi (0,2,5) sigue siendo un unico valor y no tres argumentos.
static int split_args(char *inner, char *out[], int max, PAEDProgram *p, int line) {
    int  n = 0, depth = 0, en_texto = 0;
    char *inicio = inner;

    for (char *c = inner; ; c++) {
        if (*c == '"') en_texto = !en_texto;
        if (!en_texto && (*c == '(' || *c == '[')) depth++;
        if (!en_texto && (*c == ')' || *c == ']')) depth--;

        if ((*c == '\0') || (*c == ',' && depth == 0 && !en_texto)) {
            char fin = *c;
            *c = '\0';
            char *arg = trim(inicio);
            if (*arg) {
                if (n >= max) {
                    add_error(p, line, "demasiados argumentos (maximo %d)", max);
                    return n;
                }
                out[n++] = arg;
            }
            if (fin == '\0') break;
            inicio = c + 1;
        }
    }
    return n;
}

static void parse_instruction(PAEDProgram *p, char *linea, int lineno) {
    // 1. Toda instruccion termina en ';'
    size_t len = strlen(linea);
    if (len == 0 || linea[len - 1] != ';') {
        add_error(p, lineno, "falta ';' al final de la instruccion");
        return;
    }
    linea[len - 1] = '\0';
    linea = trim(linea);

    // 2. Nombre del procedimiento, hasta el '('
    char *abre = strchr(linea, '(');
    if (!abre) {
        if (strstr(linea, ":=")) {
            add_error(p, lineno, "asignacion ':=' todavia no implementada (ver PAED.md seccion 8)");
        } else {
            add_error(p, lineno, "instruccion sin parentesis: se esperaba PROCEDIMIENTO(...)");
        }
        return;
    }

    *abre = '\0';
    char *nombre = trim(linea);
    char *inner  = abre + 1;

    if (!*nombre) {
        add_error(p, lineno, "falta el nombre del procedimiento antes de '('");
        return;
    }
    if (!es_identificador(nombre)) {
        add_error(p, lineno, "nombre de procedimiento invalido: '%s'", nombre);
        return;
    }

    // 3. Cierre de parentesis
    size_t ilen = strlen(inner);
    if (ilen == 0 || inner[ilen - 1] != ')') {
        add_error(p, lineno, "falta ')' en la llamada a %s", nombre);
        return;
    }
    inner[ilen - 1] = '\0';

    // 4. El procedimiento tiene que existir en sintaxis.json
    cJSON *def = proc_def(nombre);
    if (!def) {
        add_error(p, lineno, "procedimiento desconocido '%s' (no esta ni en %s ni en %s)",
                  nombre, PAED_SYNTAX_PATH, PAED_ESCENA_PATH);
        return;
    }

    if (p->instr_count >= PAED_MAX_INSTRS) {
        add_error(p, lineno, "demasiadas instrucciones (maximo %d)", PAED_MAX_INSTRS);
        return;
    }

    PAEDInstr *instr = &p->instrs[p->instr_count];
    memset(instr, 0, sizeof(*instr));
    strncpy(instr->proc, nombre, PAED_NAME_MAX - 1);
    instr->line = lineno;

    // 5. Argumentos
    char *partes[PAED_MAX_ARGS];
    int   n_partes = split_args(inner, partes, PAED_MAX_ARGS, p, lineno);
    int   variadico = proc_es_variadico(def);
    int   hubo_error = 0;

    for (int i = 0; i < n_partes; i++) {
        char *igual = strchr(partes[i], '=');

        if (!igual) {
            // Nada se ignora en silencio: o es variadico, o es un error.
            if (variadico) {
                strncpy(instr->args[instr->arg_count].val, partes[i], PAED_VAL_MAX - 1);
                instr->arg_count++;
                continue;
            }
            add_error(p, lineno,
                      "argumento '%s' sin 'clave = valor' en %s: la referencia siempre se escribe con nombre",
                      partes[i], nombre);
            hubo_error = 1;
            continue;
        }

        *igual = '\0';
        char *clave = trim(partes[i]);
        char *valor = trim(igual + 1);

        if (!*clave || !*valor) {
            add_error(p, lineno, "argumento incompleto en %s: se esperaba clave = valor", nombre);
            hubo_error = 1;
            continue;
        }

        cJSON *pd = param_def(def, clave);
        if (!pd && !variadico) {
            add_error(p, lineno, "parametro '%s' no existe en %s", clave, nombre);
            hubo_error = 1;
            continue;
        }

        if (pd) {
            cJSON *tipo = cJSON_GetObjectItem(pd, "tipo");
            if (cJSON_IsString(tipo) && !valida_valor(tipo->valuestring, valor)) {
                add_error(p, lineno, "'%s' no es un valor %s valido para %s de %s",
                          valor, tipo->valuestring, clave, nombre);
                hubo_error = 1;
                continue;
            }
            // Se guarda siempre con el nombre canonico, no con el alias.
            cJSON *canon = cJSON_GetObjectItem(pd, "nombre");
            if (cJSON_IsString(canon)) clave = canon->valuestring;
        }

        strncpy(instr->args[instr->arg_count].key, clave, PAED_KEY_MAX - 1);
        strncpy(instr->args[instr->arg_count].val, valor, PAED_VAL_MAX - 1);
        instr->arg_count++;
    }

    // 6. Parametros obligatorios
    cJSON *params = cJSON_GetObjectItem(def, "params");
    cJSON *pp     = NULL;
    cJSON_ArrayForEach(pp, params) {
        if (!cJSON_IsTrue(cJSON_GetObjectItem(pp, "requerido"))) continue;
        cJSON *n = cJSON_GetObjectItem(pp, "nombre");
        if (!cJSON_IsString(n)) continue;
        if (!paed_get_arg(instr, n->valuestring)) {
            add_error(p, lineno, "falta el parametro obligatorio '%s' en %s",
                      n->valuestring, nombre);
            hubo_error = 1;
        }
    }

    if (!hubo_error) p->instr_count++;
}

// ── Parseo de una declaracion del AMBIENTE: nombre : TIPO; ────────────────────

static void parse_decl(PAEDProgram *p, char *linea, int lineno) {
    size_t len = strlen(linea);
    if (len == 0 || linea[len - 1] != ';') {
        add_error(p, lineno, "falta ';' al final de la declaracion");
        return;
    }
    linea[len - 1] = '\0';

    char *dosp = strchr(linea, ':');
    if (!dosp) {
        add_error(p, lineno, "declaracion invalida: se esperaba nombre: TIPO;");
        return;
    }
    *dosp = '\0';
    char *nombre = trim(linea);
    char *tipo   = trim(dosp + 1);

    if (!es_identificador(nombre)) {
        add_error(p, lineno, "nombre de variable invalido: '%s'", nombre);
        return;
    }
    if (!*tipo) {
        add_error(p, lineno, "falta el tipo de '%s'", nombre);
        return;
    }
    if (p->decl_count >= PAED_MAX_DECLS) {
        add_error(p, lineno, "demasiadas declaraciones (maximo %d)", PAED_MAX_DECLS);
        return;
    }

    PAEDDecl *d = &p->decls[p->decl_count++];
    memset(d, 0, sizeof(*d));
    strncpy(d->name, nombre, PAED_NAME_MAX - 1);
    strncpy(d->type, tipo,   PAED_NAME_MAX - 1);
    d->line = lineno;
}

// ── Analisis del archivo completo ─────────────────────────────────────────────

typedef enum { FUERA, CABECERA, AMBIENTE, PROCESO, CERRADO } Bloque;

int paed_parse_file(const char *path, PAEDProgram *out) {
    memset(out, 0, sizeof(*out));
    strncpy(out->path, path, PAED_PATH_MAX - 1);

    if (paed_syntax_load() != 0) {
        add_error(out, 0, "no se pudo cargar la definicion del lenguaje (%s)", PAED_SYNTAX_PATH);
        return -1;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        add_error(out, 0, "no se pudo abrir el archivo");
        return -1;
    }

    char   buf[512];
    int    lineno = 0;
    Bloque bloque = FUERA;

    while (fgets(buf, sizeof(buf), f)) {
        lineno++;
        strip_comment(buf);
        char *linea = trim(buf);
        if (!*linea) continue;

        if (strncmp(linea, "ACCION", 6) == 0 && (linea[6] == ' ' || linea[6] == '\t')) {
            if (bloque != FUERA) {
                add_error(out, lineno, "ACCION anidada: un archivo .paed tiene una sola ACCION");
                continue;
            }
            char nombre[PAED_NAME_MAX] = {0}, es[8] = {0};
            if (sscanf(linea, "ACCION %63s %7s", nombre, es) != 2 || strcmp(es, "ES") != 0) {
                add_error(out, lineno, "se esperaba: ACCION <nombre> ES");
                continue;
            }
            strncpy(out->name, nombre, PAED_NAME_MAX - 1);
            bloque = CABECERA;
            continue;
        }

        if (strcmp(linea, "AMBIENTE") == 0) {
            if (bloque != CABECERA)
                add_error(out, lineno, "AMBIENTE va justo despues de ACCION ... ES y antes de PROCESO");
            bloque = AMBIENTE;
            continue;
        }

        if (strcmp(linea, "PROCESO") == 0) {
            if (bloque != CABECERA && bloque != AMBIENTE)
                add_error(out, lineno, "PROCESO fuera de lugar");
            bloque = PROCESO;
            continue;
        }

        if (strcmp(linea, "FIN_ACCION") == 0) {
            if (bloque != PROCESO)
                add_error(out, lineno, "FIN_ACCION sin un bloque PROCESO abierto");
            bloque = CERRADO;
            continue;
        }

        switch (bloque) {
            case AMBIENTE: parse_decl       (out, linea, lineno); break;
            case PROCESO:  parse_instruction(out, linea, lineno); break;
            case FUERA:
                add_error(out, lineno, "instruccion antes de ACCION");
                break;
            case CABECERA:
                add_error(out, lineno, "instruccion fuera de AMBIENTE y de PROCESO");
                break;
            case CERRADO:
                add_error(out, lineno, "instruccion despues de FIN_ACCION");
                break;
        }
    }

    fclose(f);

    if (bloque == FUERA)  add_error(out, lineno, "falta ACCION <nombre> ES");
    if (bloque == CABECERA || bloque == AMBIENTE) add_error(out, lineno, "falta PROCESO");
    if (bloque == PROCESO) add_error(out, lineno, "falta FIN_ACCION");

    return out->error_count == 0 ? 0 : -1;
}

const char *paed_get_arg(const PAEDInstr *instr, const char *key) {
    for (int i = 0; i < instr->arg_count; i++)
        if (strcmp(instr->args[i].key, key) == 0)
            return instr->args[i].val;
    return NULL;
}
