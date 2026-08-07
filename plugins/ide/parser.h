#ifndef VIMMON_PARSER_H
#define VIMMON_PARSER_H

// Parser de PAED (pseudocodigo). Un solo lenguaje, una sola gramatica.
//
// La definicion formal del lenguaje NO vive en este header: vive en
// PseudoGames/Frankly/data/sintaxis.json y se carga en runtime con cJSON.
// Si agregas un procedimiento o un parametro, se agrega ahi y nada mas.

#define PAED_NAME_MAX      64
#define PAED_KEY_MAX       32
#define PAED_VAL_MAX      128
#define PAED_MAX_ARGS      16
#define PAED_MAX_DECLS     64
#define PAED_MAX_INSTRS   256
#define PAED_MAX_ERRORS    32
#define PAED_MSG_MAX      192
#define PAED_PATH_MAX     256

// Fuente unica de verdad del LENGUAJE (pseudocodigo AED puro), relativa a la
// raiz del repo. Aca no hay nada que no este en los apuntes de la catedra.
#define PAED_SYNTAX_PATH "PseudoGames/Frankly/data/sintaxis.json"

// Libreria opcional que agrega procedimientos propios de VimMon (escena 3D).
// NO es parte del lenguaje: se carga ADEMAS de sintaxis.json. Si el archivo no
// existe, PAED sigue funcionando como AED puro.
#define PAED_ESCENA_PATH "PseudoGames/Frankly/data/escena.json"

// Argumento con nombre: clave = valor.
// En procedimientos variadicos (ESCRIBIR) key queda vacio y solo vale val.
typedef struct {
    char key[PAED_KEY_MAX];
    char val[PAED_VAL_MAX];
} PAEDArg;

// Declaracion del bloque AMBIENTE: nombre: TIPO;
typedef struct {
    char name[PAED_NAME_MAX];
    char type[PAED_NAME_MAX];
    int  line;
} PAEDDecl;

// Instruccion del bloque PROCESO: PROCEDIMIENTO(clave = valor, ...);
typedef struct {
    char    proc[PAED_NAME_MAX];
    PAEDArg args[PAED_MAX_ARGS];
    int     arg_count;
    int     line;
} PAEDInstr;

typedef struct {
    int  line;
    char msg[PAED_MSG_MAX];
} PAEDError;

typedef struct {
    char      path[PAED_PATH_MAX];
    char      name[PAED_NAME_MAX];          // el <nombre> de ACCION <nombre> ES
    PAEDDecl  decls  [PAED_MAX_DECLS];
    int       decl_count;
    PAEDInstr instrs [PAED_MAX_INSTRS];
    int       instr_count;
    PAEDError errors [PAED_MAX_ERRORS];
    int       error_count;
} PAEDProgram;

// Carga sintaxis.json. Idempotente: la segunda llamada no hace nada.
// Devuelve 0 si esta cargada, -1 si no pudo leerla.
int  paed_syntax_load(void);
void paed_syntax_free(void);

// Analiza el archivo completo. Devuelve 0 si no hubo NINGUN error.
// Si devuelve != 0, out->errors tiene el detalle: el programa no debe ejecutarse.
int  paed_parse_file(const char *path, PAEDProgram *out);

// Devuelve el valor de un argumento con nombre, o NULL si no esta.
const char *paed_get_arg(const PAEDInstr *instr, const char *key);

// Imprime todos los errores en stderr con formato archivo:linea: error: msg
void paed_print_errors(const PAEDProgram *prog);

#endif // VIMMON_PARSER_H
