#include "input.h"
#include "../../bus/plugin.h"
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>

// Guardamos la configuracion ORIGINAL de la terminal para devolverla
// intacta al apagar — si no la restauramos, la terminal del usuario
// queda rota (sin eco, sin buffer de linea) despues de que el programa termina.
static struct termios config_original;

static int input_init(void) {
    tcgetattr(STDIN_FILENO, &config_original);

    struct termios config_raw = config_original;
    config_raw.c_lflag &= ~(ICANON | ECHO); // sin esperar Enter, sin repetir la tecla en pantalla
    tcsetattr(STDIN_FILENO, TCSANOW, &config_raw);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); // read() no bloquea si no hay tecla ni click

    // Activa el reporte de mouse de la terminal: 1000 = reporte basico de
    // click, 1006 = modo SGR (coordenadas extendidas, mas facil de parsear).
    printf("\x1b[?1000h\x1b[?1006h");
    fflush(stdout);

    printf("[input] iniciado (teclado + mouse)\n");
    return 0;
}

static void input_shutdown(void) {
    // Apaga el reporte de mouse — si no, la terminal del usuario sigue
    // mandando secuencias de escape por cada click despues de que el
    // programa termina, rompiendo la shell igual que el modo raw sin restaurar.
    printf("\x1b[?1000l\x1b[?1006l");
    fflush(stdout);

    tcsetattr(STDIN_FILENO, TCSANOW, &config_original); // terminal como estaba antes
    printf("[input] apagado\n");
}

// Un reporte de mouse llega en varios bytes seguidos (no como una tecla
// suelta), asi que si el primer byte no esta completo todavia reintentamos
// un ratito antes de rendirnos — evita cortar la secuencia a la mitad.
static int leer_byte_con_reintento(char *out) {
    for (int intento = 0; intento < 100; intento++) {
        ssize_t leidos = read(STDIN_FILENO, out, 1);
        if (leidos > 0) return 1;
        usleep(200);
    }
    return 0; // se agoto el reintento, secuencia incompleta o basura
}

// Parsea un reporte SGR de mouse: ESC [ < Cb ; Cx ; Cy (M o m)
// Cb = boton, Cx/Cy = columna/fila, M = press, m = release.
static void procesar_secuencia_mouse(void) {
    char c;

    if (!leer_byte_con_reintento(&c) || c != '[') return;
    if (!leer_byte_con_reintento(&c) || c != '<') return; // no es SGR, la descartamos

    int cb = 0, cx = 0, cy = 0;

    while (leer_byte_con_reintento(&c) && c != ';') {
        if (c < '0' || c > '9') return;
        cb = cb * 10 + (c - '0');
    }
    while (leer_byte_con_reintento(&c) && c != ';') {
        if (c < '0' || c > '9') return;
        cx = cx * 10 + (c - '0');
    }

    char terminador = 0;
    while (leer_byte_con_reintento(&c)) {
        if (c == 'M' || c == 'm') { terminador = c; break; }
        if (c < '0' || c > '9') return;
        cy = cy * 10 + (c - '0');
    }
    if (terminador == 0) return;

    MouseEvent evento = {
        .x = cx,
        .y = cy,
        .boton = cb & 3,
        .presionado = (terminador == 'M') ? 1 : 0,
    };

    printf("[input] mouse: boton=%d x=%d y=%d %s\n",
           evento.boton, evento.x, evento.y,
           evento.presionado ? "presionado" : "liberado");

    bus_send(EVENT_MOUSE, &evento, sizeof(evento));
}

static void input_tick(float delta) {
    (void)delta;

    char c;
    ssize_t leidos = read(STDIN_FILENO, &c, 1);
    if (leidos <= 0) return;

    if (c == 0x1b) { // ESC: puede ser el inicio de un reporte de mouse
        procesar_secuencia_mouse();
        return;
    }

    printf("[input] tecla capturada: '%c' (0x%02x)\n", c, c);

    KeyboardEvent tecla = { .tecla = c };
    bus_send(EVENT_KEYBOARD, &tecla, sizeof(tecla));
}

static void input_on_event(Event *e) { (void)e; }

Plugin input_plugin = {
    .name      = "input",
    .version   = "0.1",
    .init      = input_init,
    .shutdown  = input_shutdown,
    .tick      = input_tick,
    .on_event  = input_on_event,
};
