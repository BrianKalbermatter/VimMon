#ifndef VIMMON_EXPR_H
#define VIMMON_EXPR_H

#include <stddef.h>   // size_t
#include "parser.h"

// Evaluador de expresiones de PAED.
//
// El parser guarda las condiciones y las asignaciones CRUDAS, como texto. Este
// modulo es el que las convierte en un valor. Sin el, el parser sabe que
// `MIENTRAS (cont < 4) HACER` es un bucle y a donde saltar, pero no sabe si
// hay que entrar.
//
// Es un descenso recursivo: una funcion por nivel de prioridad, y cada una
// llama a la de mayor prioridad que ella. La tabla sale de
// TEORIA_COMPLETA.txt:361-371 (de MENOR a mayor prioridad aca abajo):
//
//     O                 <- la funcion mas externa
//     Y
//     =  <>
//     <  <=  >  >=
//     +  -               (suma y resta)
//     *  /  DIV  MOD
//     **                 (potencia, asociativa a derecha)
//     + - NO             (unarios)
//     literales, variables, funciones, ( )
//
// Que la prioridad sea el ORDEN DE LAS LLAMADAS y no una tabla de numeros es
// lo que hace que "2 + 3 * 4" de 14 y no 20: cuando suma() pide sus operandos,
// producto() ya se comio el "3 * 4".

#define PAED_MAX_VARS 64

typedef enum {
    VAL_NUM,      // entero o real: en PAED no se distinguen al evaluar
    VAL_TEXTO,
    VAL_LOGICO,
} TipoValor;

typedef struct {
    TipoValor tipo;
    double    num;                 // VAL_NUM
    int       logico;              // VAL_LOGICO: 0 o 1
    char      texto[PAED_VAL_MAX]; // VAL_TEXTO
} Valor;

typedef struct {
    char  nombre[PAED_NAME_MAX];
    Valor valor;
} Variable;

// Tabla de variables. Sin malloc, como todo el resto del proyecto.
typedef struct {
    Variable items[PAED_MAX_VARS];
    int      count;
    char     error[PAED_MSG_MAX];  // vacio si la ultima operacion salio bien
} Entorno;

void   env_init  (Entorno *e);
Valor *env_buscar(Entorno *e, const char *nombre);   // NULL si no existe
int    env_set   (Entorno *e, const char *nombre, Valor v);  // 0 OK, -1 lleno

// Evalua `texto` y deja el resultado en `out`.
// Devuelve 0 si salio bien, -1 si no: el motivo queda en env->error.
int expr_eval(const char *texto, Entorno *env, Valor *out);

// Lee un valor como condicion. Un NUM cuenta como falso solo si es 0, y un
// texto vacio cuenta como falso: asi una condicion nunca queda "indefinida".
int valor_verdadero(const Valor *v);

// Escribe el valor en `out` con formato legible (para ESCRIBIR).
void valor_a_texto(const Valor *v, char *out, size_t out_size);

#endif // VIMMON_EXPR_H
