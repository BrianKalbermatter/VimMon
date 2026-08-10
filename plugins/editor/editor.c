#include "editor.h"
#include "../ide/parser.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Puente entre el bus y el editorBim. Lo que hace de verdad:
//
//   1. se asegura de que el archivo exista (el editor lo lee con mapfile,
//      que falla si no esta)
//   2. lanza el editor en su propio directorio
//   3. cuando vuelve, si el archivo es .paed, lo parsea y muestra los errores
//
// El paso 3 es lo que lo hace parte del OS y no un lanzador: salis del editor
// y el sistema ya te dice si lo que escribiste compila.

// El editorBim hace `source ./keys.sh` con ruta RELATIVA, asi que hay que
// pararse en su carpeta antes de ejecutarlo.
#define EDITOR_DIR    "paed/scripts/editorBim"
#define EDITOR_SCRIPT "./bim.sh"

// Se puede cambiar el editor sin recompilar:
//
//     VIMMON_EDITOR=/usr/bin/vim build/vimmon
//
// Sirve para probar el resto de la cadena sin abrir un editor interactivo, y
// para el dia que haya un segundo editor. Si no esta, se usa el editorBim.
#define EDITOR_ENV "VIMMON_EDITOR"

static int editor_init(void)     { printf("[editor] iniciado\n"); return 0; }
static void editor_shutdown(void) { printf("[editor] apagado\n"); }
static void editor_tick(float d)  { (void)d; }

// El editor abre el archivo con `mapfile < "$1"`, que falla si no existe.
// Crearlo vacio primero permite usar 'edit' para EMPEZAR un archivo, no solo
// para modificar uno que ya esta.
static int asegurar_archivo(const char *ruta) {
    if (access(ruta, F_OK) == 0) return 0;

    FILE *f = fopen(ruta, "w");
    if (!f) {
        fprintf(stderr, "[editor] no se pudo crear %s: %s\n", ruta, strerror(errno));
        return -1;
    }
    fclose(f);
    printf("[editor] %s no existia, se creo vacio\n", ruta);
    return 0;
}

// ¿Termina en .paed? Solo esos se validan al salir.
static int es_paed(const char *ruta) {
    size_t n = strlen(ruta);
    return n > 5 && strcmp(ruta + n - 5, ".paed") == 0;
}

// Parsea el archivo y reporta. No ejecuta nada: salir del editor no deberia
// correr el programa, solo decir si esta bien escrito.
static void validar(const char *ruta) {
    PAEDProgram prog;
    if (paed_parse_file(ruta, &prog) == 0) {
        printf("[editor] %s: sin errores (%d instrucciones)\n", ruta, prog.instr_count);
        return;
    }
    printf("[editor] %s tiene %d error(es):\n", ruta, prog.error_count);
    // El detalle va a stderr, que no tiene buffer. Sin vaciar stdout primero,
    // los errores aparecen ANTES del encabezado que se acaba de imprimir.
    fflush(stdout);
    paed_print_errors(&prog);
}

static void abrir(const char *ruta) {
    if (!ruta || !*ruta) {
        fprintf(stderr, "[editor] falta el archivo: edit <archivo>\n");
        return;
    }

    // La ruta se resuelve ANTES de cambiar de directorio: el editor corre desde
    // su propia carpeta, asi que una ruta relativa apuntaria a otro lado.
    char absoluta[PAED_PATH_MAX];
    if (ruta[0] == '/') {
        snprintf(absoluta, sizeof(absoluta), "%s", ruta);
    } else {
        char cwd[PAED_PATH_MAX];
        if (!getcwd(cwd, sizeof(cwd))) {
            fprintf(stderr, "[editor] no se pudo leer el directorio actual\n");
            return;
        }
        snprintf(absoluta, sizeof(absoluta), "%s/%s", cwd, ruta);
    }

    if (asegurar_archivo(absoluta) != 0) return;

    printf("[editor] abriendo %s\n", absoluta);
    fflush(stdout);   // el editor toma la terminal: que salga esto antes

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "[editor] no se pudo lanzar: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        // Hijo: lanza el editor y no vuelve nunca (execl lo reemplaza).
        const char *otro = getenv(EDITOR_ENV);
        if (otro && *otro) {
            execl(otro, otro, absoluta, (char *)NULL);
            fprintf(stderr, "[editor] no se pudo ejecutar %s: %s\n", otro, strerror(errno));
            _exit(127);
        }

        // El editorBim se busca desde su propia carpeta, por los `source ./`.
        if (chdir(EDITOR_DIR) != 0) {
            fprintf(stderr, "[editor] no se pudo entrar a %s: %s\n",
                    EDITOR_DIR, strerror(errno));
            _exit(127);
        }
        execl(EDITOR_SCRIPT, "bim.sh", absoluta, (char *)NULL);
        // Si execl vuelve, fallo.
        fprintf(stderr, "[editor] no se pudo ejecutar %s/%s: %s\n",
                EDITOR_DIR, EDITOR_SCRIPT, strerror(errno));
        _exit(127);
    }

    int estado = 0;
    waitpid(pid, &estado, 0);

    // El editor pone la terminal en modo raw. Si sale bien la restaura solo
    // (tiene un trap), pero si lo matan a mitad la deja rota y hay que escribir
    // 'stty sane' a ciegas. Se restaura igual por las dudas.
    // Solo si hay terminal: con la entrada redirigida (un script, una tuberia)
    // stty falla con "Inappropriate ioctl for device" y ensucia la salida sin
    // que haya nada que restaurar.
    if (isatty(STDIN_FILENO) && system("stty sane") == -1)
        fprintf(stderr, "[editor] ojo: no se pudo restaurar la terminal, escribi 'stty sane'\n");

    if (WIFEXITED(estado) && WEXITSTATUS(estado) == 127) {
        fprintf(stderr, "[editor] el editor no arranco\n");
        return;
    }

    printf("\n[editor] de vuelta en vimmon\n");
    if (es_paed(absoluta)) validar(absoluta);
}

static void editor_on_event(Event *e) {
    switch (e->type) {
    case EVENT_EDITOR_OPEN:
        abrir((const char *)e->data);
        break;
    default:
        break;
    }
}

Plugin editor_plugin = {
    .name     = "editor",
    .version  = "0.1",
    .init     = editor_init,
    .shutdown = editor_shutdown,
    .tick     = editor_tick,
    .on_event = editor_on_event,
};
