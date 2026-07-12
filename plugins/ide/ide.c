#include "ide.h"
#include "parser.h"
#include "interpreter.h"
#include "../ai/ai.h"
#include <stdio.h>

static SceneState scene;

static int ide_init(void) {
    interp_init(&scene);
    printf("[ide] iniciado\n");
    return 0;
}

static void ide_shutdown(void) {
    printf("[ide] apagado\n");
}

static void ide_tick(float delta) { (void)delta; }

static void ide_on_event(Event *e) {
    if (e->type != EVENT_AI_RESPONSE) return;

    AIResponse *resp = (AIResponse *)e->data;

    FILE *f = fopen(SCENE_PATH, "a");
    if (!f) {
        fprintf(stderr, "[ide] no se pudo abrir scene.paed\n");
        return;
    }
    fprintf(f, "\n%s\n", resp->paed_delta);
    fclose(f);

    PAEDScene paed;
    if (paed_parse_file(SCENE_PATH, &paed) != 0) {
        fprintf(stderr, "[ide] error al parsear scene.paed\n");
        return;
    }

    interp_init(&scene);
    interp_exec(&scene, &paed);

    printf("[ide] escena actualizada:\n");
    interp_print(&scene);

    bus_send(EVENT_SCENE_UPDATE, &scene, sizeof(scene));
}

Plugin ide_plugin = {
    .name     = "ide",
    .version  = "0.1",
    .init     = ide_init,
    .shutdown = ide_shutdown,
    .tick     = ide_tick,
    .on_event = ide_on_event,
};
