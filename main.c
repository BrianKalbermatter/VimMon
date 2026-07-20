#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bus/plugin.h"
#include "plugins/ai/ai.h"
#include "plugins/ide/ide.h"
#include "plugins/ide/parser.h"
#include "plugins/ide/interpreter.h"
#include "plugins/keyboard/keyboard.h"

extern Plugin ai_plugin;
extern Plugin renderer_plugin;
extern Plugin monitor_plugin;

// corriendo controla el loop principal. sig_atomic_t porque lo toca un
// signal handler (puede interrumpir el programa en cualquier punto).
static volatile sig_atomic_t corriendo = 1;

void manejar_señal(int señal) {
    (void)señal; // no nos importa cual señal fue, solo que llegó
    corriendo = 0;
}

int main(void) {
    printf("=== VimMon OS arrancando ===\n\n");

    signal(SIGINT, manejar_señal); // Ctrl+C apaga limpio, no mata el proceso a la fuerza

    bus_init(); // Inicio el collector event

    EventType ai_inputs[]       = { EVENT_AI_REQUEST };
    EventType ide_inputs[]      = { EVENT_AI_RESPONSE };
    EventType renderer_inputs[] = { EVENT_SCENE_UPDATE, EVENT_RENDER_FRAME };
    EventType monitor_inputs[]  = { EVENT_MONITOR_TICK };
    // keyboard no necesita recibir nada — solo lo suscribimos a
    // EVENT_SHUTDOWN para que bus_shutdown() lo encuentre y lo apague.
    EventType keyboard_inputs[] = { EVENT_SHUTDOWN };

    bus_register(&ai_plugin,       ai_inputs,       1);
    ai_plugin.init();

    bus_register(&ide_plugin,      ide_inputs,      1);
    ide_plugin.init();

    bus_register(&renderer_plugin, renderer_inputs, 2);
    renderer_plugin.init();

    bus_register(&monitor_plugin,  monitor_inputs,  1);
    monitor_plugin.init();

    bus_register(&keyboard_plugin, keyboard_inputs, 1);
    keyboard_plugin.init();

    printf("\n[main] todos los plugins listos.\n\n");

    // Test FASE 2 — parser + interprete directo (sin Ollama)
    printf("[test] cargando scene.paed...\n");
    PAEDScene paed;
    SceneState scene;
    interp_init(&scene);
    if (paed_parse_file(SCENE_PATH, &paed) == 0) {
        interp_exec(&scene, &paed);
        interp_print(&scene);
        printf("[test] OK — interprete sin crash\n\n");
    } else {
        printf("[test] ERROR — no se pudo leer scene.paed\n\n");
    }

    // Test FASE 1 — mandar un request a la IA
    AIRequest req;
    strncpy(req.prompt, "creá un cubo rojo en el centro de la escena", AI_PROMPT_MAX - 1);
    req.prompt[AI_PROMPT_MAX - 1] = '\0';
    bus_send(EVENT_AI_REQUEST, &req, sizeof(req));

    printf("\n[main] loop principal arrancando (Ctrl+C para salir)...\n\n");
    while (corriendo) {
        ai_plugin.tick(0.0f);
        ide_plugin.tick(0.0f);
        renderer_plugin.tick(0.0f);
        monitor_plugin.tick(0.0f);
        keyboard_plugin.tick(0.0f);

        usleep(100000); // ~10 vueltas por segundo, no hay nada que renderizar todavia
    }

    printf("\n[main] señal de salida recibida, apagando...\n");
    bus_shutdown();
    printf("\n=== VimMon OS apagado ===\n");

    return 0;
}
