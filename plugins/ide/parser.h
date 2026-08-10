#ifndef VIMMON_PARSER_H
#define VIMMON_PARSER_H

// Parser de PAED (pseudocodigo). Un solo lenguaje, una sola gramatica.
//
// La definicion formal del lenguaje NO vive en este header: vive en
// paed/Frankly/data/sintaxis.json y se carga en runtime con cJSON.
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
#define PAED_COND_MAX     192
// Cuantos bloques se pueden anidar. 32 niveles de SI dentro de MIENTRAS dentro
// de SI es mucho mas de lo que aguanta leer un humano.
#define PAED_MAX_BLOQUES   32

// Fuente unica de verdad del LENGUAJE (pseudocodigo AED puro), relativa a la
// raiz del repo. Aca no hay nada que no este en los apuntes de la catedra.
#define PAED_SYNTAX_PATH "paed/Frankly/data/sintaxis.json"

// Archivo de estado de la escena: lo que el interprete ejecuta y lo que el
// motor dibuja. Vive al lado del resto de las rutas de PAED, no en ai.h: la
// escena es del IDE, la IA es solo uno de los que la escribe.
#define PAED_SCENE_PATH "plugins/ide/scene.paed"

// Libreria opcional que agrega procedimientos propios de VimMon (escena 3D).
// NO es parte del lenguaje: se carga ADEMAS de sintaxis.json. Si el archivo no
// existe, PAED sigue funcionando como AED puro.
#define PAED_ESCENA_PATH "paed/Frankly/data/escena.json"

// Argumento con nombre: clave = valor.
// En procedimientos variadicos (ESCRIBIR) key queda vacio y solo vale val.
typedef struct {
    char key[PAED_KEY_MAX];
    char val[PAED_VAL_MAX];
} PAEDArg;

// Declaracion del bloque AMBIENTE: nombre: TIPO;
// Tambien   nombre: ARREGLO[desde..hasta] DE TIPO;
typedef struct {
    char name[PAED_NAME_MAX];
    char type[PAED_NAME_MAX];   // el tipo BASE: ENTERO, REAL, CARACTER, LOGICO
    int  es_arreglo;
    // Limites del arreglo, los dos inclusive. En AED los elige el programador
    // y no arrancan en 0: ARREGLO[1..10] va del 1 al 10.
    int  desde, hasta;
    int  line;
} PAEDDecl;

// Que clase de instruccion es. Antes toda linea del PROCESO era una llamada;
// con los bloques hay lineas que no llaman a nada (SINO, FIN_SI) y otras que
// llevan una condicion en vez de argumentos.
typedef enum {
    PAED_LLAMADA = 0,     // PROCEDIMIENTO(clave = valor, ...);
    PAED_ASIGNA,          // destino := expresion;
    PAED_SI,              // SI (condicion) ENTONCES
    PAED_SINO,            // SINO
    PAED_FIN_SI,          // FIN_SI
    PAED_MIENTRAS,        // MIENTRAS (condicion) HACER
    PAED_FIN_MIENTRAS,    // FIN_MIENTRAS
    PAED_PARA,            // PARA <var> := <desde> HASTA <hasta> HACER
    PAED_FIN_PARA,        // FIN_PARA
} PAEDKind;

// Instruccion del bloque PROCESO.
typedef struct {
    PAEDKind kind;
    // LLAMADA: nombre del proc. ASIGNA: destino. PARA: la variable del bucle,
    // con los limites en args como desde/hasta.
    char    proc[PAED_NAME_MAX];
    PAEDArg args[PAED_MAX_ARGS];
    int     arg_count;
    int     line;

    // Condicion de SI/MIENTRAS, o la expresion de la derecha de un ':='.
    // Se guarda CRUDA: todavia no hay evaluador de expresiones.
    char    cond[PAED_COND_MAX];

    // A donde salta el flujo. Lo completa el parser cuando CIERRA el bloque,
    // porque al abrirlo todavia no sabe donde termina (esto se llama
    // "backpatching"). Vale -1 en las instrucciones que no saltan.
    //   SI            -> primera instruccion del SINO, o la de despues del FIN_SI
    //   SINO          -> la de despues del FIN_SI
    //   MIENTRAS      -> la de despues del FIN_MIENTRAS (cuando la condicion es falsa)
    //   FIN_MIENTRAS  -> el MIENTRAS de arriba (para volver a evaluar)
    int     salto;
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
