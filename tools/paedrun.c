// paedrun — corre un programa PAED desde la terminal, sin abrir la ventana.
//
// El interprete vive dentro del game loop, asi que hasta ahora la unica forma
// de probar un .paed era abrir SDL y mirar. Eso no se puede automatizar ni
// meter en un script: un test que hay que mirar no es un test.
//
// Este arnes linkea SOLO el parser, el evaluador y el interprete. No toca SDL
// ni el bus. ESCRIBIR ya imprime por stdout, asi que la salida se puede
// comparar contra un archivo esperado.
//
//   build/paedrun paed/Frankly/tests/busqueda_lineal.paed
//
// Codigos de salida, para poder encadenarlo con && en la shell:
//   0  el programa corrio entero
//   1  error de parseo o de ejecucion
//   2  mal uso
//   3  no se pudo cargar sintaxis.json

#include <stdio.h>
#include <string.h>

#include "plugins/ide/parser.h"
#include "plugins/ide/interpreter.h"

// De aca saca LEER sus datos. El interprete no abre stdin solo (ver el
// comentario de interp_set_entrada): en la ventana SDL eso congelaria el game
// loop. Aca, en cambio, stdin es exactamente lo que corresponde — sea el que
// tipea o una tuberia con los datos del test.
//
// Una linea mas larga que el buffer se parte: lo que sobra queda como la
// proxima linea, y el proximo LEER se lo come. Es el comportamiento de fgets y
// se prefiere a truncar callado.
static int leer_de_stdin(char *buf, size_t n, void *ud) {
    (void)ud;
    if (!fgets(buf, (int)n, stdin)) return -1;   // fin de la entrada
    buf[strcspn(buf, "\n")] = '\0';
    return 0;
}

int main(int argc, char **argv) {
    int mostrar_escena = 0;
    const char *path   = NULL;

    // ESCRIBIR va a stdout y los errores a stderr. Cuando la salida es una
    // tuberia y no la terminal, stdout pasa a tener buffer y stderr no: los
    // errores se adelantan y aparecen ANTES de lineas que en realidad se
    // imprimieron primero. Sin esto, comparar la salida contra un archivo
    // esperado da diferencias que dependen de si hay tuberia o no.
    setvbuf(stdout, NULL, _IONBF, 0);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--escena") == 0) mostrar_escena = 1;
        else if (!path)                       path = argv[i];
        else {
            fprintf(stderr, "paedrun: argumento de mas: %s\n", argv[i]);
            return 2;
        }
    }

    if (!path) {
        fprintf(stderr, "uso: paedrun [--escena] <archivo.paed>\n");
        return 2;
    }

    // sintaxis.json se busca relativo a la raiz del repo, no al .paed: hay que
    // correr paedrun parado en la raiz.
    if (paed_syntax_load() != 0) {
        fprintf(stderr, "paedrun: no se pudo cargar %s "
                        "(hay que correrlo desde la raiz del repo)\n", PAED_SYNTAX_PATH);
        return 3;
    }

    PAEDProgram prog;
    if (paed_parse_file(path, &prog) != 0) {
        paed_print_errors(&prog);
        paed_syntax_free();
        return 1;
    }

    SceneState scene;
    interp_init(&scene);
    interp_set_entrada(leer_de_stdin, NULL);
    int rc = interp_exec(&scene, &prog);

    if (mostrar_escena) interp_print(&scene);

    paed_syntax_free();
    return rc == 0 ? 0 : 1;
}
