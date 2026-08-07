#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// El parser ya valido nombres, tipos y obligatorios contra sintaxis.json.
// Aca solo queda ejecutar: buscar la entidad y aplicar el efecto.

static Vec3 parse_vec3(const char *s) {
    Vec3 v = {0, 0, 0};
    if (s) sscanf(s, "(%f,%f,%f)", &v.x, &v.y, &v.z);
    return v;
}

static float parse_num(const char *s) {
    return s ? (float)atof(s) : 0.0f;
}

// Los parametros marcados "requerido" ya los valido el parser, pero nunca se
// desreferencia un puntero que vino de una tabla editable a mano.
static const char *arg_o(const PAEDInstr *in, const char *key, const char *por_defecto) {
    const char *v = paed_get_arg(in, key);
    return v ? v : por_defecto;
}

static void runtime_error(const PAEDProgram *prog, const PAEDInstr *in, const char *msg) {
    fprintf(stderr, "%s:%d: error: %s\n", prog->path, in->line, msg);
}

static Entity *find_entity(SceneState *scene, const char *id) {
    if (!id) return NULL;
    for (int i = 0; i < scene->entity_count; i++)
        if (strcmp(scene->entities[i].id, id) == 0)
            return &scene->entities[i];
    return NULL;
}

static Entity *find_or_create(SceneState *scene, const char *id, const char *kind) {
    Entity *e = find_entity(scene, id);
    if (e) {
        strncpy(e->kind, kind, sizeof(e->kind) - 1);
        return e;
    }
    if (scene->entity_count >= SCENE_MAX_ENTITIES) return NULL;

    e = &scene->entities[scene->entity_count++];
    memset(e, 0, sizeof(*e));
    strncpy(e->id,   id,   sizeof(e->id)   - 1);
    strncpy(e->kind, kind, sizeof(e->kind) - 1);
    e->scale.x = e->scale.y = e->scale.z = 1.0f;
    strncpy(e->color, "#ffffff", sizeof(e->color) - 1);
    return e;
}

// Devuelve la entidad referenciada por nombre=<id>, o NULL reportando el error.
static Entity *entidad_referida(SceneState *scene, const PAEDProgram *prog, const PAEDInstr *in) {
    const char *id = paed_get_arg(in, "nombre");
    Entity     *e  = find_entity(scene, id);
    if (!e) {
        char msg[PAED_MSG_MAX];
        snprintf(msg, sizeof(msg), "la entidad '%s' no existe en la escena", id ? id : "?");
        runtime_error(prog, in, msg);
    }
    return e;
}

static int exec_instr(SceneState *scene, const PAEDProgram *prog, const PAEDInstr *in) {
    const char *p = in->proc;

    // ── Globales ─────────────────────────────────────────────────────────────
    if (strcmp(p, "FONDO") == 0) {
        strncpy(scene->bg_color, arg_o(in, "color", "#000000"), sizeof(scene->bg_color) - 1);
        return 0;
    }
    if (strcmp(p, "CAMARA") == 0) {
        const char *pos   = paed_get_arg(in, "posicion");
        const char *mirar = paed_get_arg(in, "mirar");
        if (pos)   scene->cam_pos    = parse_vec3(pos);
        if (mirar) scene->cam_target = parse_vec3(mirar);
        return 0;
    }

    // ── Creacion de entidades ────────────────────────────────────────────────
    int es_cubo   = strcmp(p, "CUBO")   == 0;
    int es_esfera = strcmp(p, "ESFERA") == 0;
    int es_plano  = strcmp(p, "PLANO")  == 0;
    int es_luz    = strcmp(p, "LUZ")    == 0;

    if (es_cubo || es_esfera || es_plano || es_luz) {
        const char *kind = es_cubo ? "cubo" : es_esfera ? "esfera" : es_plano ? "plano" : "luz";
        const char *id = arg_o(in, "nombre", "");
        if (!*id) {
            runtime_error(prog, in, "falta nombre = <id>");
            return -1;
        }
        Entity *e = find_or_create(scene, id, kind);
        if (!e) {
            runtime_error(prog, in, "la escena esta llena, no entran mas entidades");
            return -1;
        }

        const char *pos    = paed_get_arg(in, "posicion");
        const char *color  = paed_get_arg(in, "color");
        const char *tamano = paed_get_arg(in, "tamano");
        const char *radio  = paed_get_arg(in, "radio");
        const char *tipo   = paed_get_arg(in, "tipo");
        const char *inten  = paed_get_arg(in, "intensidad");

        if (pos)    e->position   = parse_vec3(pos);
        if (color)  strncpy(e->color, color, sizeof(e->color) - 1);
        if (tamano) e->scale      = parse_vec3(tamano);
        if (radio)  e->radio      = parse_num(radio);
        if (tipo)   strncpy(e->luz_tipo, tipo, sizeof(e->luz_tipo) - 1);
        if (inten)  e->intensidad = parse_num(inten);
        return 0;
    }

    // ── Modificaciones ───────────────────────────────────────────────────────
    if (strcmp(p, "MOVER") == 0) {
        Entity *e = entidad_referida(scene, prog, in);
        if (!e) return -1;
        e->position = parse_vec3(arg_o(in, "posicion", "(0,0,0)"));
        return 0;
    }

    if (strcmp(p, "ROTAR") == 0) {
        Entity *e = entidad_referida(scene, prog, in);
        if (!e) return -1;
        float grados = parse_num(paed_get_arg(in, "angulo"));
        switch (arg_o(in, "eje", "")[0]) {
            case 'x': e->rotation.x = grados; break;
            case 'y': e->rotation.y = grados; break;
            case 'z': e->rotation.z = grados; break;
            default:  runtime_error(prog, in, "eje invalido: se espera x, y o z"); return -1;
        }
        return 0;
    }

    if (strcmp(p, "ESCALAR") == 0) {
        Entity *e = entidad_referida(scene, prog, in);
        if (!e) return -1;
        float f = parse_num(paed_get_arg(in, "factor"));
        e->scale.x *= f;
        e->scale.y *= f;
        e->scale.z *= f;
        return 0;
    }

    if (strcmp(p, "COLOR") == 0) {
        Entity *e = entidad_referida(scene, prog, in);
        if (!e) return -1;
        strncpy(e->color, arg_o(in, "color", "#ffffff"), sizeof(e->color) - 1);
        return 0;
    }

    // ── Comportamientos ──────────────────────────────────────────────────────
    if (strcmp(p, "GIRAR") == 0) {
        Entity *e = entidad_referida(scene, prog, in);
        if (!e) return -1;
        char eje = arg_o(in, "eje", "")[0];
        if (eje != 'x' && eje != 'y' && eje != 'z') {
            runtime_error(prog, in, "eje invalido: se espera x, y o z");
            return -1;
        }
        e->giro_eje       = eje;
        e->giro_velocidad = parse_num(paed_get_arg(in, "velocidad"));
        return 0;
    }

    if (strcmp(p, "OSCILAR") == 0) {
        Entity *e = entidad_referida(scene, prog, in);
        if (!e) return -1;
        e->osc_amplitud   = parse_num(paed_get_arg(in, "amplitud"));
        e->osc_frecuencia = parse_num(paed_get_arg(in, "frecuencia"));
        return 0;
    }

    // ── Salida ───────────────────────────────────────────────────────────────
    if (strcmp(p, "ESCRIBIR") == 0) {
        for (int i = 0; i < in->arg_count; i++) {
            const char *v = in->args[i].val;
            size_t n = strlen(v);
            if (n >= 2 && v[0] == '"' && v[n - 1] == '"')
                printf("%.*s", (int)(n - 2), v + 1);   // texto literal, sin comillas
            else
                printf("%s", v);
        }
        printf("\n");
        return 0;
    }

    runtime_error(prog, in, "procedimiento reconocido por el parser pero no implementado todavia");
    return -1;
}

void interp_init(SceneState *scene) {
    memset(scene, 0, sizeof(*scene));
    strncpy(scene->bg_color, "#000000", sizeof(scene->bg_color) - 1);
}

int interp_exec(SceneState *scene, const PAEDProgram *prog) {
    int fallos = 0;
    for (int i = 0; i < prog->instr_count; i++)
        if (exec_instr(scene, prog, &prog->instrs[i]) != 0) fallos++;
    return fallos == 0 ? 0 : -1;
}

void interp_print(const SceneState *scene) {
    printf("[scene] fondo=%s  cam=(%.1f,%.1f,%.1f)\n",
           scene->bg_color,
           scene->cam_pos.x, scene->cam_pos.y, scene->cam_pos.z);

    for (int i = 0; i < scene->entity_count; i++) {
        const Entity *e = &scene->entities[i];
        printf("  [%s] %-10s pos=(%.1f,%.1f,%.1f)  color=%s",
               e->kind, e->id,
               e->position.x, e->position.y, e->position.z,
               e->color);
        if (e->radio > 0.0f)      printf("  radio=%.2f", e->radio);
        if (e->giro_eje)          printf("  gira en %c a %.2f", e->giro_eje, e->giro_velocidad);
        if (e->osc_frecuencia)    printf("  oscila %.2f/%.2f", e->osc_amplitud, e->osc_frecuencia);
        printf("\n");
    }
}
