#include "ide.h"
#include "parser.h"
#include "interpreter.h"
#include "../ai/ai.h"
#include <stdio.h>
#include <string.h>

static SceneState scene;

static int ide_init(void) {
    interp_init(&scene);
    printf("[ide] iniciado\n");
    return 0;
}

static void ide_shutdown(void) {
    paed_syntax_free();
    printf("[ide] apagado\n");
}

static void ide_tick(float delta) { (void)delta; }

// PAED es un lenguaje con bloques: el delta de la IA son instrucciones sueltas,
// asi que NO se puede appendear al final del archivo (quedaria despues de
// FIN_ACCION). Se reescribe el archivo insertando el delta antes del cierre.
static int insertar_delta(const char *path, const char *delta) {
    char lineas[PAED_MAX_INSTRS][512];
    int  total = 0, corte = -1;

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    while (total < PAED_MAX_INSTRS && fgets(lineas[total], sizeof(lineas[0]), f)) {
        if (strstr(lineas[total], "FIN_ACCION")) corte = total;
        total++;
    }
    fclose(f);

    if (corte < 0) {
        fprintf(stderr, "[ide] %s no tiene FIN_ACCION, no se donde insertar\n", path);
        return -1;
    }

    f = fopen(path, "w");
    if (!f) return -1;

    for (int i = 0; i < corte; i++) fputs(lineas[i], f);
    fprintf(f, "\n        %s\n", delta);
    for (int i = corte; i < total; i++) fputs(lineas[i], f);

    fclose(f);
    return 0;
}

static void ide_on_event(Event *e) {
    if (e->type != EVENT_AI_RESPONSE) return;

    AIResponse *resp = (AIResponse *)e->data;

    if (insertar_delta(SCENE_PATH, resp->paed_delta) != 0) {
        fprintf(stderr, "[ide] no se pudo escribir el delta en %s\n", SCENE_PATH);
        return;
    }

    PAEDProgram prog;
    if (paed_parse_file(SCENE_PATH, &prog) != 0) {
        fprintf(stderr, "[ide] %s tiene errores, la escena NO se actualizo:\n", SCENE_PATH);
        paed_print_errors(&prog);
        return;
    }

    interp_init(&scene);
    interp_exec(&scene, &prog);

    printf("[ide] escena actualizada:\n");
    interp_print(&scene);

    bus_send(EVENT_SCENE_UPDATE, &scene, sizeof(scene));
}

Plugin ide_plugin = {
    .name     = "ide",
    .version  = "0.2",
    .init     = ide_init,
    .shutdown = ide_shutdown,
    .tick     = ide_tick,
    .on_event = ide_on_event,
};
