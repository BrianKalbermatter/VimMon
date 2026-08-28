// ============================================================
// build/mundo — corre un .paed que dibuja en 3D.
//
// Es el HOST: hospeda al interprete de PAED y le agrega la libreria `mundo`.
// Se usa igual que el `paed` normal, pero con ventana:
//
//   ./build/mundo LaberintoMinotauro/laberinto.paed
//
// El orden de abajo NO es negociable y vale la pena entenderlo:
//
//   1. parsear el .paed     cada 'USAR x;' carga x.json ahi mismo
//   2. registrar los verbos ahora los nombres reciben un cuerpo en C
//   3. ejecutar
//
// Declarar (json) e implementar (C) son dos cosas distintas y pasan en momentos
// distintos. Sin el USAR da "CUBO no existe" en el parser; sin el registro da
// "lo reconoce el parser pero no lo implementa nadie" en runtime.
// ============================================================

#include "pl_mundo.h"

#include "paed/parser.h"
#include "paed/interpreter.h"

#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "uso: %s <programa.paed>\n", argv[0]);
        return 2;
    }

    // 1. El programa PIDE sus librerias con USAR, asi que el host ya no elige
    //    ninguna: el parser las carga solo al leer cada linea USAR.
    //
    //    Es un cambio de quien manda. Antes el host decidia con que vocabulario
    //    se leia el archivo; ahora lo dice el archivo, que es el unico que sabe
    //    lo que necesita.
    // paed_parse_file JUNTA los errores en prog; imprimirlos es del que hospeda.
    // Sin esta linea el host falla mudo: devuelve 1 y no dice por que, que es el
    // peor error posible — el que no deja rastro.
    static PAEDProgram prog;
    if (paed_parse_file(argv[1], &prog) != 0) {
        paed_print_errors(&prog);
        return 1;
    }

    // 2. Los nombres reciben su cuerpo en C.
    if (mundo_registrar() != 0) {
        fprintf(stderr, "[mundo] no entraron todos los verbos en el registro\n");
        return 1;
    }

    // 3. A correr.
    int rc = interp_exec(&prog);

    // Apagar el motor va PRIMERO y no es un detalle de prolijidad: si el proceso
    // termina con la ventana y los recursos de GPU todavia vivos, muere con
    // segfault DESPUES de haber corrido bien todo el programa. Un error que
    // aparece al final y no tiene nada que ver con el final.
    mundo_apagar();
    paed_clear_procs();
    paed_syntax_free();
    return rc == 0 ? 0 : 1;
}
