#ifndef VIMMON_AI_H
#define VIMMON_AI_H

#include "../../bus/plugin.h"

#define AI_PROMPT_MAX    512
#define AI_RESPONSE_MAX  4096
#define AI_MODEL         "llama3.2:3b"
#define AI_ENDPOINT      "http://localhost:11434/api/generate"
#define SCENE_PATH       "plugins/ide/scene.paed"

typedef struct {
    char prompt[AI_PROMPT_MAX];
} AIRequest;

typedef struct {
    char paed_delta[AI_RESPONSE_MAX];
} AIResponse;

extern Plugin ai_plugin;

#endif // VIMMON_AI_H
