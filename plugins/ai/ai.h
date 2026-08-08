#ifndef VIMMON_AI_H
#define VIMMON_AI_H

#include "../../bus/plugin.h"

#define AI_PROMPT_MAX    512
#define AI_RESPONSE_MAX  4096
#define SCENE_PATH       "plugins/ide/scene.paed"

// El modelo y el endpoint ya NO viven aca: los define la tabla de provider.c y
// se eligen en runtime con 'ai use'. Tener una sola fuente de verdad evita que
// el header diga una cosa y la tabla otra.

typedef struct {
    char prompt[AI_PROMPT_MAX];
} AIRequest;

typedef struct {
    char paed_delta[AI_RESPONSE_MAX];
} AIResponse;

extern Plugin ai_plugin;

#endif // VIMMON_AI_H
