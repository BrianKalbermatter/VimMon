#include "editor.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

// Puente entre el bus y PseudoGames, el IDE completo.
//
// PseudoGames NO es un editor de un archivo: es la aplicacion entera, con su
// menu, sus niveles, la wiki, el pomodoro y el editor adentro. Se abre como se
// abre el motor con 'engine': el comando publica un evento, este plugin lo
// agarra y lanza el programa, y cuando se cierra volves a la consola.
//
// El plugin NO es el IDE. El IDE lo escribe el usuario y vive en paed/.
// Aca solo esta la conexion.

// PseudoGames carga assets/, data/ y saves/ con rutas RELATIVAS, asi que tiene
// que correr parado en su propia carpeta.
#define IDE_DIR    "paed"
#define IDE_BIN    "./aed"
#define IDE_BIN_FS "paed/aed"

// Cambiar el programa sin recompilar VimMon:
//     VIMMON_IDE=/usr/bin/vim build/vimmon
#define IDE_ENV "VIMMON_IDE"

static int  editor_init(void)     { printf("[editor] iniciado\n"); return 0; }
static void editor_shutdown(void) { printf("[editor] apagado\n"); }
static void editor_tick(float d)  { (void)d; }

// PseudoGames se compila aparte, con su propio Makefile. Si el binario no esta,
// se compila en vez de largar "no existe": la primera vez que alguien escribe
// 'edit' no tiene por que saber que habia que ir a compilar a mano.
static int asegurar_binario(void) {
    if (access(IDE_BIN_FS, X_OK) == 0) return 0;

    printf("[editor] %s no esta compilado, compilando...\n", IDE_BIN_FS);
    fflush(stdout);

    int r = system("make -C " IDE_DIR " 2>&1 | tail -3");
    if (r != 0 || access(IDE_BIN_FS, X_OK) != 0) {
        fprintf(stderr,
                "[editor] no se pudo compilar PseudoGames.\n"
                "         Probá a mano:  make -C %s\n"
                "         Necesita SDL2, SDL2_ttf, SDL2_mixer y SDL2_image.\n",
                IDE_DIR);
        return -1;
    }
    printf("[editor] compilado\n");
    return 0;
}

static void abrir(void) {
    const char *otro = getenv(IDE_ENV);

    if (!otro && asegurar_binario() != 0) return;

    printf("[editor] abriendo PseudoGames...\n");
    fflush(stdout);   // la app toma la pantalla: que salga esto antes

    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "[editor] no se pudo lanzar: %s\n", strerror(errno));
        return;
    }

    if (pid == 0) {
        // Hijo: se convierte en el IDE y no vuelve nunca.
        if (otro && *otro) {
            execl(otro, otro, (char *)NULL);
            fprintf(stderr, "[editor] no se pudo ejecutar %s: %s\n", otro, strerror(errno));
            _exit(127);
        }
        if (chdir(IDE_DIR) != 0) {
            fprintf(stderr, "[editor] no se pudo entrar a %s: %s\n",
                    IDE_DIR, strerror(errno));
            _exit(127);
        }
        execl(IDE_BIN, "aed", (char *)NULL);
        fprintf(stderr, "[editor] no se pudo ejecutar %s/%s: %s\n",
                IDE_DIR, IDE_BIN, strerror(errno));
        _exit(127);
    }

    int estado = 0;
    waitpid(pid, &estado, 0);

    // La app puede dejar la terminal rara si la matan a mitad. Solo se restaura
    // si hay terminal de verdad: con la entrada redirigida, stty falla con
    // "Inappropriate ioctl for device" y ensucia la salida sin arreglar nada.
    if (isatty(STDIN_FILENO) && system("stty sane") == -1)
        fprintf(stderr, "[editor] ojo: si la terminal quedó rara, escribí 'stty sane'\n");

    if (WIFEXITED(estado) && WEXITSTATUS(estado) == 127) {
        fprintf(stderr, "[editor] PseudoGames no arrancó\n");
        return;
    }
    if (WIFSIGNALED(estado)) {
        fprintf(stderr, "\n[editor] PseudoGames terminó por señal %d\n", WTERMSIG(estado));
        return;
    }

    printf("\n[editor] PseudoGames cerrado, de vuelta en vimmon\n");
}

static void editor_on_event(Event *e) {
    switch (e->type) {
    case EVENT_EDITOR_OPEN:
        abrir();
        break;
    default:
        break;
    }
}

Plugin editor_plugin = {
    .name     = "editor",
    .version  = "0.2",
    .init     = editor_init,
    .shutdown = editor_shutdown,
    .tick     = editor_tick,
    .on_event = editor_on_event,
};
