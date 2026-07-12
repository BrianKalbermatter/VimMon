#ifndef VIMMON_INTERPRETER_H
#define VIMMON_INTERPRETER_H

#include "parser.h"

#define SCENE_MAX_ENTITIES 64

typedef struct { float x, y, z; } Vec3;

typedef struct {
    char id[32];
    char kind[32];
    Vec3 position;
    Vec3 rotation;
    Vec3 scale;
    char color[16];
} Entity;

typedef struct {
    Entity entities[SCENE_MAX_ENTITIES];
    int    entity_count;
    char   bg_color[16];
    Vec3   cam_pos;
    Vec3   cam_target;
} SceneState;

void interp_init (SceneState *scene);
int  interp_exec (SceneState *scene, const PAEDScene *paed);
void interp_print(const SceneState *scene);

#endif // VIMMON_INTERPRETER_H
