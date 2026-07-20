#include "keyboard.h"
#include "../../bus/plugin.h"
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

// Guardamos la configuracion ORIGINAL de la terminal para devolverla
// intacta al apagar — si no la restauramos, la terminal del usuario
// queda rota (sin eco, sin buffer de linea) despues de que el programa termina.
static struct termios config_original;

static int keyboard_init(void) {
    tcgetattr(STDIN_FILENO, &config_original);

    struct termios config_raw = config_original;
    config_raw.c_lflag &= ~(ICANON | ECHO); // sin esperar Enter, sin repetir la tecla en pantalla
    tcsetattr(STDIN_FILENO, TCSANOW, &config_raw);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); // read() no bloquea si no hay tecla

    printf("[keyboard] iniciado\n");
    return 0;
}

static void keyboard_shutdown(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &config_original); // terminal como estaba antes
    printf("[keyboard] apagado\n");
}

static void keyboard_tick(float delta) {
    (void)delta;

    char c;
    ssize_t leidos = read(STDIN_FILENO, &c, 1);

    if (leidos > 0) {
        printf("[keyboard] tecla capturada: '%c' (0x%02x)\n", c, c);

        KeyboardEvent tecla = { .tecla = c };
        bus_send(EVENT_KEYBOARD, &tecla, sizeof(tecla));
    }
}

static void keyboard_on_event(Event *e) { (void)e; }

Plugin keyboard_plugin = {
    .name      = "keyboard",
    .version   = "0.1",
    .init      = keyboard_init,
    .shutdown  = keyboard_shutdown,
    .tick      = keyboard_tick,
    .on_event  = keyboard_on_event,
};
