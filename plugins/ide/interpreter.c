#include "interpreter.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static Vec3 parse_vec3(const char *s) {
    Vec3 v = {0};
    if (s) sscanf(s, "%f,%f,%f", &v.x, &v.y, &v.z);
    return v;
}

static Entity *find_or_create(SceneState *scene, const char *id, const char *kind) {
    for (int i = 0; i < scene->entity_count; i++)
        if (strcmp(scene->entities[i].id, id) == 0)
            return &scene->entities[i];

    if (scene->entity_count >= SCENE_MAX_ENTITIES) return NULL;

    Entity *e = &scene->entities[scene->entity_count++];
    memset(e, 0, sizeof(*e));
    strncpy(e->id,   id,   sizeof(e->id)   - 1);
    strncpy(e->kind, kind, sizeof(e->kind) - 1);
    e->scale.x = e->scale.y = e->scale.z = 1.0f;
    return e;
}

static Entity *find_entity(SceneState *scene, const char *id) {
    for (int i = 0; i < scene->entity_count; i++)
        if (strcmp(scene->entities[i].id, id) == 0)
            return &scene->entities[i];
    return NULL;
}

static void exec_line(SceneState *scene, const PAEDLine *line) {
    const char *type = line->type;

    if (strcmp(type, "fondo") == 0) {
        const char *c = paed_get_param(line, "color");
        if (c) strncpy(scene->bg_color, c, sizeof(scene->bg_color) - 1);
        return;
    }

    if (strcmp(type, "camara") == 0) {
        const char *pos    = paed_get_param(line, "posicion");
        const char *target = paed_get_param(line, "mirar");
        if (pos)    scene->cam_pos    = parse_vec3(pos);
        if (target) scene->cam_target = parse_vec3(target);
        return;
    }

    if (strcmp(type, "cubo")   == 0 ||
        strcmp(type, "esfera") == 0 ||
        strcmp(type, "plano")  == 0 ||
        strcmp(type, "luz")    == 0) {
        const char *id  = paed_get_param(line, "nombre");
        if (!id) return;
        Entity *e = find_or_create(scene, id, type);
        if (!e) return;
        const char *pos   = paed_get_param(line, "posicion");
        const char *color = paed_get_param(line, "color");
        const char *scale = paed_get_param(line, "tamaño");
        if (pos)   e->position = parse_vec3(pos);
        if (color) strncpy(e->color, color, sizeof(e->color) - 1);
        if (scale) e->scale    = parse_vec3(scale);
        return;
    }

    if (strcmp(type, "mover") == 0) {
        const char *id  = paed_get_param(line, "nombre");
        const char *pos = paed_get_param(line, "posicion");
        Entity *e = find_entity(scene, id);
        if (e && pos) e->position = parse_vec3(pos);
        return;
    }

    if (strcmp(type, "rotar") == 0) {
        const char *id  = paed_get_param(line, "nombre");
        const char *rot = paed_get_param(line, "angulo");
        Entity *e = find_entity(scene, id);
        if (e && rot) e->rotation = parse_vec3(rot);
        return;
    }

    if (strcmp(type, "color") == 0) {
        const char *id    = paed_get_param(line, "nombre");
        const char *color = paed_get_param(line, "color");
        Entity *e = find_entity(scene, id);
        if (e && color) strncpy(e->color, color, sizeof(e->color) - 1);
        return;
    }
}

void interp_init(SceneState *scene) {
    memset(scene, 0, sizeof(*scene));
    strncpy(scene->bg_color, "#000000", sizeof(scene->bg_color) - 1);
}

int interp_exec(SceneState *scene, const PAEDScene *paed) {
    for (int i = 0; i < paed->line_count; i++)
        exec_line(scene, &paed->lines[i]);
    return 0;
}

void interp_print(const SceneState *scene) {
    printf("[scene] fondo=%s  cam=(%.1f,%.1f,%.1f)\n",
           scene->bg_color,
           scene->cam_pos.x, scene->cam_pos.y, scene->cam_pos.z);
    for (int i = 0; i < scene->entity_count; i++) {
        const Entity *e = &scene->entities[i];
        printf("  [%s] %s  pos=(%.1f,%.1f,%.1f)  color=%s\n",
               e->kind, e->id,
               e->position.x, e->position.y, e->position.z,
               e->color);
    }
}
