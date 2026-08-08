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

// Reserva la proxima instruccion ya inicializada. salto = -1 significa "esta no
// salta a ningun lado"; las de bloque lo completan cuando se cierra el bloque.
static PAEDInstr *nueva_instr(PAEDProgram *p, PAEDKind kind, int lineno) {
    if (p->instr_count >= PAED_MAX_INSTRS) {
        add_error(p, lineno, "demasiadas instrucciones (maximo %d)", PAED_MAX_INSTRS);
        return NULL;
    }
    PAEDInstr *in = &p->instrs[p->instr_count++];
    memset(in, 0, sizeof(*in));
    in->kind  = kind;
    in->line  = lineno;
    in->salto = -1;
    return in;
}

// destino := expresion;   (el ';' ya se saco)
// La expresion se guarda CRUDA: todavia no hay evaluador. Igual se valida que
// el destino sea un identificador y que haya algo del lado derecho, porque
// aceptar basura ahora es esconder el error para mas adelante.
static void parse_asignacion(PAEDProgram *p, char *linea, int lineno, char *op) {
    *op = '\0';
    char *destino = trim(linea);
    char *expr    = trim(op + 2);

    if (!es_identificador(destino)) {
        add_error(p, lineno, "destino de asignacion invalido: '%s'", destino);
        return;
    }
    if (!*expr) {
        add_error(p, lineno, "falta la expresion a la derecha de ':=' en '%s'", destino);
        return;
    }

    PAEDInstr *in = nueva_instr(p, PAED_ASIGNA, lineno);
    if (!in) return;
    strncpy(in->proc, destino, PAED_NAME_MAX - 1);
    strncpy(in->cond, expr,    PAED_COND_MAX - 1);
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

    // 2. ¿Es una asignacion? Lo es si hay ':=' y aparece ANTES del primer '(',
    //    para que 'x := f(y);' cuente como asignacion y 'AVZ(a, b);' no.
    char *abre     = strchr(linea, '(');
    char *asignaop = strstr(linea, ":=");
    if (asignaop && (!abre || asignaop < abre)) {
        parse_asignacion(p, linea, lineno, asignaop);
        return;
    }

    // 3. Nombre del procedimiento, hasta el '('
    if (!abre) {
        add_error(p, lineno, "instruccion sin parentesis: se esperaba PROCEDIMIENTO(...)");
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

    // Se llena la ranura sin consumirla todavia: si algo falla mas abajo,
    // instr_count no avanza y la instruccion rota no queda en el programa.
    PAEDInstr *instr = &p->instrs[p->instr_count];
    memset(instr, 0, sizeof(*instr));
    instr->kind  = PAED_LLAMADA;
    instr->salto = -1;
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

// ── La pila de bloques ────────────────────────────────────────────────────────
//
// Una variable sola no sirve para el anidamiento. En
//
//     MIENTRAS (a) HACER          MIENTRAS (a) HACER
//         MIENTRAS (b) HACER          x := 1;
//         FIN_MIENTRAS            FIN_MIENTRAS
//     FIN_MIENTRAS                MIENTRAS (b) HACER
//                                 FIN_MIENTRAS
//
// los dos programas tienen 2 MIENTRAS y 2 FIN_MIENTRAS: contar no los
// distingue. Hace falta saber QUIEN abrio cada uno.
//
// Y es una PILA y no otra cosa porque los bloques cierran en el orden inverso
// al que se abrieron: el ultimo que se abrio es el primero que se cierra. Eso
// es LIFO, y eso es exactamente una pila. Es lo mismo que hace un compilador
// de C con las llaves.

typedef struct {
    PAEDKind kind;   // PAED_SI o PAED_MIENTRAS
    int      line;   // donde se abrio, para poder decirlo en el error
    int      instr;  // que instruccion lo abrio, para parchearle el salto
    int      sino;   // indice del SINO si ya aparecio; -1 si no
} Abierto;

typedef struct {
    Abierto items[PAED_MAX_BLOQUES];
    int     tope;
} Pila;

static const char *nombre_kind(PAEDKind k) {
    return k == PAED_SI ? "SI" : "MIENTRAS";
}

// Saca la palabra clave del principio y el terminador del final.
// "SI (a = 1) ENTONCES" -> "(a = 1)". NULL si falta el terminador.
static char *cuerpo_cabecera(char *linea, const char *kw, const char *fin) {
    char  *c  = trim(linea + strlen(kw));
    size_t lc = strlen(c), nf = strlen(fin);

    if (lc < nf || strcmp(c + lc - nf, fin) != 0) return NULL;
    // El terminador tiene que ser una palabra suelta, no el final de otra:
    // sin esto, un identificador que termine en HACER pasaria por terminador.
    if (lc > nf && !isspace((unsigned char)c[lc - nf - 1]) && c[lc - nf - 1] != ')')
        return NULL;

    c[lc - nf] = '\0';
    return trim(c);
}

// ¿La linea empieza con esta palabra clave como palabra entera?
static int empieza_con(const char *linea, const char *kw) {
    size_t n = strlen(kw);
    if (strncmp(linea, kw, n) != 0) return 0;
    return linea[n] == '\0' || isspace((unsigned char)linea[n]) || linea[n] == '(';
}

// Abre un bloque: crea la instruccion, guarda la condicion y apila.
static void abrir(PAEDProgram *p, Pila *pila, PAEDKind kind,
                  const char *cond, int lineno) {
    PAEDInstr *in = nueva_instr(p, kind, lineno);
    if (!in) return;
    strncpy(in->cond, cond, PAED_COND_MAX - 1);

    if (pila->tope >= PAED_MAX_BLOQUES) {
        add_error(p, lineno, "demasiados bloques anidados (maximo %d)", PAED_MAX_BLOQUES);
        return;
    }
    pila->items[pila->tope++] = (Abierto){ kind, lineno, p->instr_count - 1, -1 };
}

// Devuelve 1 si la linea era de bloque (y ya la trato), 0 si no lo era.
static int parse_bloque(PAEDProgram *p, char *linea, int lineno, Pila *pila) {

    // ── SI (condicion) ENTONCES ──
    if (empieza_con(linea, "SI")) {
        char *cond = cuerpo_cabecera(linea, "SI", "ENTONCES");
        if (!cond)  { add_error(p, lineno, "se esperaba: SI <condicion> ENTONCES"); return 1; }
        if (!*cond) { add_error(p, lineno, "el SI no tiene condicion"); return 1; }
        abrir(p, pila, PAED_SI, cond, lineno);
        return 1;
    }

    // ── MIENTRAS (condicion) HACER ──
    if (empieza_con(linea, "MIENTRAS")) {
        char *cond = cuerpo_cabecera(linea, "MIENTRAS", "HACER");
        if (!cond)  { add_error(p, lineno, "se esperaba: MIENTRAS <condicion> HACER"); return 1; }
        if (!*cond) { add_error(p, lineno, "el MIENTRAS no tiene condicion"); return 1; }
        abrir(p, pila, PAED_MIENTRAS, cond, lineno);
        return 1;
    }

    // ── SINO ──
    if (strcmp(linea, "SINO") == 0) {
        if (pila->tope == 0 || pila->items[pila->tope - 1].kind != PAED_SI) {
            add_error(p, lineno, "SINO sin un SI abierto");
            return 1;
        }
        Abierto *a = &pila->items[pila->tope - 1];
        if (a->sino >= 0) {
            add_error(p, lineno, "SINO repetido: el SI de la linea %d ya tiene uno", a->line);
            return 1;
        }
        PAEDInstr *in = nueva_instr(p, PAED_SINO, lineno);
        if (!in) return 1;
        int idx = p->instr_count - 1;

        // Condicion falsa -> arrancar justo despues del SINO.
        p->instrs[a->instr].salto = idx + 1;
        a->sino = idx;
        return 1;
    }

    // ── FIN_SI ──
    if (strcmp(linea, "FIN_SI") == 0) {
        if (pila->tope == 0) { add_error(p, lineno, "FIN_SI sin un SI abierto"); return 1; }
        Abierto *a = &pila->items[pila->tope - 1];
        if (a->kind != PAED_SI) {
            // Este es el mensaje que la pila hace posible: se puede decir QUE
            // bloque quedo abierto y EN QUE LINEA.
            add_error(p, lineno, "FIN_SI cierra un %s abierto en la linea %d",
                      nombre_kind(a->kind), a->line);
            return 1;
        }
        PAEDInstr *in = nueva_instr(p, PAED_FIN_SI, lineno);
        if (!in) return 1;
        int idx = p->instr_count - 1;

        // Con SINO, el que salta al final es el SINO (el SI ya apunta a el).
        // Sin SINO, es el propio SI el que se saltea el cuerpo.
        if (a->sino >= 0) p->instrs[a->sino].salto  = idx + 1;
        else              p->instrs[a->instr].salto = idx + 1;

        pila->tope--;
        return 1;
    }

    // ── FIN_MIENTRAS ──
    if (strcmp(linea, "FIN_MIENTRAS") == 0) {
        if (pila->tope == 0) { add_error(p, lineno, "FIN_MIENTRAS sin un MIENTRAS abierto"); return 1; }
        Abierto *a = &pila->items[pila->tope - 1];
        if (a->kind != PAED_MIENTRAS) {
            add_error(p, lineno, "FIN_MIENTRAS cierra un %s abierto en la linea %d",
                      nombre_kind(a->kind), a->line);
            return 1;
        }
        PAEDInstr *in = nueva_instr(p, PAED_FIN_MIENTRAS, lineno);
        if (!in) return 1;
        int idx = p->instr_count - 1;

        in->salto                 = a->instr;  // volver a evaluar la condicion
        p->instrs[a->instr].salto = idx + 1;   // condicion falsa -> salir del bucle

        pila->tope--;
        return 1;
    }

    return 0;   // no era una linea de bloque
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
    Pila   pila   = { .tope = 0 };   // bloques abiertos dentro del PROCESO

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
            // FIN_ACCION no puede cerrar bloques que quedaron abiertos: se avisa
            // aca, con la linea de apertura, y no cuando ya no se sabe nada.
            for (int i = pila.tope - 1; i >= 0; i--)
                add_error(out, lineno, "falta FIN_%s: el %s de la linea %d quedo abierto",
                          nombre_kind(pila.items[i].kind),
                          nombre_kind(pila.items[i].kind), pila.items[i].line);
            pila.tope = 0;
            bloque = CERRADO;
            continue;
        }

        switch (bloque) {
            case AMBIENTE: parse_decl(out, linea, lineno); break;
            case PROCESO:
                // Primero los bloques: sus cabeceras NO llevan ';', asi que
                // tienen que reconocerse antes de que parse_instruction lo exija.
                if (!parse_bloque(out, linea, lineno, &pila))
                    parse_instruction(out, linea, lineno);
                break;
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
