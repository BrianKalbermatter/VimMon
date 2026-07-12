#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bus/bus.h"
#include "plugins/ai/ai.h"
#include "plugins/ide/ide.h"
#include "plugins/ide/parser.h"
#include "plugins/ide/interpreter.h"

extern Plugin ai_plugin;
extern Plugin renderer_plugin;
extern Plugin monitor_plugin;

int main(void) {
    printf("=== VimMon OS arrancando ===\n\n");

    bus_init(); // Inicio el collector event

    EventType ai_inputs[]       = { EVENT_AI_REQUEST };
    EventType ide_inputs[]      = { EVENT_AI_RESPONSE };
    EventType renderer_inputs[] = { EVENT_SCENE_UPDATE, EVENT_RENDER_FRAME };
    EventType monitor_inputs[]  = { EVENT_MONITOR_TICK };

    bus_register(&ai_plugin,       ai_inputs,       1);
    ai_plugin.init();

    bus_register(&ide_plugin,      ide_inputs,      1);
    ide_plugin.init();

    bus_register(&renderer_plugin, renderer_inputs, 2);
    renderer_plugin.init();

    bus_register(&monitor_plugin,  monitor_inputs,  1);
    monitor_plugin.init();

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

    printf("\n[main] apagando...\n");
    bus_shutdown();
    printf("\n=== VimMon OS apagado ===\n");

    return 0;
}
