#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "bus/plugin.h"
#include "plugins/ai/ai.h"
#include "plugins/ide/ide.h"
#include "plugins/ide/parser.h"
#include "plugins/ide/interpreter.h"

extern Plugin ai_plugin;
extern Plugin renderer_plugin;
extern Plugin monitor_plugin;

// corriendo controla la consola. sig_atomic_t porque lo toca un
// signal handler (puede interrumpir el programa en cualquier punto).
static volatile sig_atomic_t corriendo = 1;

void manejar_señal(int señal) {
    (void)señal; // no nos importa cuál señal fue, solo que llegó
    corriendo = 0;
}

// ── Comandos de la consola ────────────────────────────────────

static void cmd_help(void) {
    printf("\nComandos disponibles:\n");
    printf("  engine   abre el motor gráfico con tu juego (game/game.c)\n");
    printf("  scene    carga y ejecuta plugins/ide/scene.paed (intérprete PAED)\n");
    printf("  ai       manda un prompt de prueba a Ollama (necesita Ollama activo)\n");
    printf("  help     muestra esta ayuda\n");
    printf("  quit     apaga vimmon\n\n");
}

// engine: publica el evento; el renderer_plugin lo agarra y abre el motor.
static void cmd_engine(void) {
    bus_send(EVENT_RENDER_FRAME, NULL, 0);
}

// scene: parsea scene.paed y lo ejecuta en el intérprete PAED.
static void cmd_scene(void) {
    PAEDProgram prog;
    SceneState  scene;
    interp_init(&scene);

    // Analisis primero: si hay un solo error, no se ejecuta nada.
    if (paed_parse_file(SCENE_PATH, &prog) != 0) {
        paed_print_errors(&prog);
        printf("[scene] ERROR — %d error(es), no se ejecuto\n", prog.error_count);
        return;
    }

    int ok = interp_exec(&scene, &prog) == 0;
    interp_print(&scene);
    printf("[scene] %s — %s (%d instrucciones)\n",
           ok ? "OK" : "con errores de ejecucion", prog.name, prog.instr_count);
}

// ai: manda un request al plugin de IA por el bus (procesa en on_event).
static void cmd_ai(void) {
    AIRequest req;
    strncpy(req.prompt, "creá un cubo rojo en el centro de la escena",
            AI_PROMPT_MAX - 1);
    req.prompt[AI_PROMPT_MAX - 1] = '\0';
    bus_send(EVENT_AI_REQUEST, &req, sizeof(req));
}

// Despacha una línea ya sin salto de línea. Devuelve 0 si hay que salir.
static int despachar(const char *cmd) {
    if (cmd[0] == '\0')                 return 1;   // línea vacía
    if (strcmp(cmd, "engine") == 0)     { cmd_engine(); return 1; }
    if (strcmp(cmd, "scene")  == 0)     { cmd_scene();  return 1; }
    if (strcmp(cmd, "ai")     == 0)     { cmd_ai();     return 1; }
    if (strcmp(cmd, "help")   == 0)     { cmd_help();   return 1; }
    if (strcmp(cmd, "quit")   == 0 ||
        strcmp(cmd, "exit")   == 0)     return 0;     // salir

    printf("comando desconocido: '%s' (probá 'help')\n", cmd);
    return 1;
}

int main(void) {
    printf("=== VimMon OS arrancando ===\n\n");

    signal(SIGINT, manejar_señal); // Ctrl+C apaga limpio

    bus_init();

    EventType ai_inputs[]       = { EVENT_AI_REQUEST };
    EventType ide_inputs[]      = { EVENT_AI_RESPONSE };
    EventType renderer_inputs[] = { EVENT_SCENE_UPDATE, EVENT_RENDER_FRAME };
    EventType monitor_inputs[]  = { EVENT_MONITOR_TICK };

    bus_register(&ai_plugin,       ai_inputs,       1);  ai_plugin.init();
    bus_register(&ide_plugin,      ide_inputs,      1);  ide_plugin.init();
    bus_register(&renderer_plugin, renderer_inputs, 2);  renderer_plugin.init();
    bus_register(&monitor_plugin,  monitor_inputs,  1);  monitor_plugin.init();

    // El plugin 'input' queda DORMIDO a propósito: su init() pone la
    // terminal en modo raw + no-bloqueante, y eso se pelea con la consola
    // (fgets necesita modo línea normal). Cuando quieras input por terminal,
    // se reactiva. Dentro del juego, el teclado ya lo maneja SDL, no este plugin.

    printf("\n[main] todos los plugins listos. Escribí 'help' para ver los comandos.\n\n");

    // Consola: leé un comando, despachalo, repetí. Corta con 'quit',
    // Ctrl+D (fin de entrada) o Ctrl+C.
    char linea[256];
    while (corriendo) {
        printf("vimmon> ");
        fflush(stdout);

        if (fgets(linea, sizeof(linea), stdin) == NULL)
            break;                              // Ctrl+D / fin de entrada

        linea[strcspn(linea, "\n")] = '\0';     // sacar el salto de línea

        if (!despachar(linea))
            break;                              // el comando pidió salir
    }

    printf("\n[main] apagando...\n");
    bus_shutdown();
    printf("\n=== VimMon OS apagado ===\n");
    return 0;
}
