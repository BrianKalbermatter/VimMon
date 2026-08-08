#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   // sinf/cosf: rotar un grupo alrededor de su centro

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

static Cuerpo *find_cuerpo(SceneState *scene, const char *id) {
    if (!id) return NULL;
    for (int i = 0; i < scene->cuerpo_count; i++)
        if (strcmp(scene->cuerpos[i].id, id) == 0)
            return &scene->cuerpos[i];
    return NULL;
}

static Cuerpo *find_or_create(SceneState *scene, const char *id, const char *kind) {
    Cuerpo *e = find_cuerpo(scene, id);
    if (e) {
        strncpy(e->kind, kind, sizeof(e->kind) - 1);
        return e;
    }
    if (scene->cuerpo_count >= SCENE_MAX_CUERPOS) return NULL;

    e = &scene->cuerpos[scene->cuerpo_count++];
    memset(e, 0, sizeof(*e));
    strncpy(e->id,   id,   sizeof(e->id)   - 1);
    strncpy(e->kind, kind, sizeof(e->kind) - 1);
    e->scale.x = e->scale.y = e->scale.z = 1.0f;
    strncpy(e->color, "#ffffff", sizeof(e->color) - 1);
    return e;
}

// ── Objetivo de una modificacion ─────────────────────────────────────────────
// nombre=<x> puede referirse a UNA entidad o a un GRUPO entero. Resolverlo en
// un solo lugar evita que cada procedimiento invente su propia regla.
typedef struct {
    Cuerpo *items[SCENE_MAX_CUERPOS];
    int     count;
    int     es_grupo;   // 1 si <x> nombro un grupo, 0 si nombro una pieza suelta
} Objetivo;

// La pieza gana sobre el grupo: si existe una entidad con ese id exacto, es esa.
// Asi podes tocar 'ala_izq' sin mover la nave entera.
static int resolver(SceneState *scene, const PAEDProgram *prog,
                    const PAEDInstr *in, Objetivo *obj) {
    const char *id = paed_get_arg(in, "nombre");
    obj->count = 0;
    obj->es_grupo = 0;

    Cuerpo *e = find_cuerpo(scene, id);
    if (e) {
        obj->items[obj->count++] = e;
        return 0;
    }

    if (id) {
        for (int i = 0; i < scene->cuerpo_count; i++)
            if (strcmp(scene->cuerpos[i].grupo, id) == 0)
                obj->items[obj->count++] = &scene->cuerpos[i];
    }

    if (obj->count > 0) { obj->es_grupo = 1; return 0; }

    char msg[PAED_MSG_MAX];
    snprintf(msg, sizeof(msg), "no existe entidad ni grupo '%s' en la escena", id ? id : "?");
    runtime_error(prog, in, msg);
    return -1;
}

// Centro del grupo: el promedio de las posiciones. Es el punto respecto del
// cual se mueve, rota y escala el conjunto, para que no se deforme.
static Vec3 centro(const Objetivo *obj) {
    Vec3 c = {0, 0, 0};
    if (obj->count == 0) return c;
    for (int i = 0; i < obj->count; i++) {
        c.x += obj->items[i]->position.x;
        c.y += obj->items[i]->position.y;
        c.z += obj->items[i]->position.z;
    }
    c.x /= (float)obj->count;
    c.y /= (float)obj->count;
    c.z /= (float)obj->count;
    return c;
}

// Gira un punto alrededor de un eje que pasa por 'c'. Rotacion 2D de toda la
// vida aplicada al plano perpendicular al eje.
static Vec3 girar_alrededor(Vec3 p, Vec3 c, char eje, float grados) {
    float r = grados * 3.14159265f / 180.0f;
    float s = sinf(r), co = cosf(r);
    float a, b;

    switch (eje) {
        case 'x':
            a = p.y - c.y; b = p.z - c.z;
            p.y = c.y + a * co - b * s;
            p.z = c.z + a * s  + b * co;
            break;
        case 'y':
            a = p.x - c.x; b = p.z - c.z;
            p.x = c.x + a * co + b * s;
            p.z = c.z - a * s  + b * co;
            break;
        case 'z':
            a = p.x - c.x; b = p.y - c.y;
            p.x = c.x + a * co - b * s;
            p.y = c.y + a * s  + b * co;
            break;
    }
    return p;
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
        Cuerpo *e = find_or_create(scene, id, kind);
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
        const char *grupo  = paed_get_arg(in, "grupo");

        if (grupo)  strncpy(e->grupo, grupo, sizeof(e->grupo) - 1);
        if (pos)    e->position   = parse_vec3(pos);
        if (color)  strncpy(e->color, color, sizeof(e->color) - 1);
        if (tamano) e->scale      = parse_vec3(tamano);
        if (radio)  e->radio      = parse_num(radio);
        if (tipo)   strncpy(e->luz_tipo, tipo, sizeof(e->luz_tipo) - 1);
        if (inten)  e->intensidad = parse_num(inten);
        return 0;
    }

    // ── Modificaciones ───────────────────────────────────────────────────────
    // Todas resuelven primero el objetivo: puede ser una pieza o un grupo.
    Objetivo obj;

    if (strcmp(p, "MOVER") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        Vec3 destino = parse_vec3(arg_o(in, "posicion", "(0,0,0)"));

        if (!obj.es_grupo) {
            obj.items[0]->position = destino;
            return 0;
        }
        // En grupo, MOVER es TRASLADAR: si le pusieramos la misma posicion a
        // cada pieza, la nave se derrumbaria en un punto. Movemos el centro y
        // arrastramos a todos con el mismo delta.
        Vec3 c = centro(&obj);
        for (int i = 0; i < obj.count; i++) {
            obj.items[i]->position.x += destino.x - c.x;
            obj.items[i]->position.y += destino.y - c.y;
            obj.items[i]->position.z += destino.z - c.z;
        }
        return 0;
    }

    if (strcmp(p, "ROTAR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        float grados = parse_num(paed_get_arg(in, "angulo"));
        char  eje    = arg_o(in, "eje", "")[0];
        if (eje != 'x' && eje != 'y' && eje != 'z') {
            runtime_error(prog, in, "eje invalido: se espera x, y o z");
            return -1;
        }

        Vec3 c = centro(&obj);
        for (int i = 0; i < obj.count; i++) {
            Cuerpo *e = obj.items[i];
            // El grupo gira alrededor de su centro; una pieza suelta gira sobre si misma.
            if (obj.es_grupo) e->position = girar_alrededor(e->position, c, eje, grados);
            switch (eje) {
                case 'x': e->rotation.x = grados; break;
                case 'y': e->rotation.y = grados; break;
                case 'z': e->rotation.z = grados; break;
            }
        }
        return 0;
    }

    if (strcmp(p, "ESCALAR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        float f = parse_num(paed_get_arg(in, "factor"));

        Vec3 c = centro(&obj);
        for (int i = 0; i < obj.count; i++) {
            Cuerpo *e = obj.items[i];
            e->scale.x *= f;
            e->scale.y *= f;
            e->scale.z *= f;
            // Agrandar las piezas sin separarlas las encimaria: en un grupo
            // tambien se estiran las distancias al centro.
            if (obj.es_grupo) {
                e->position.x = c.x + (e->position.x - c.x) * f;
                e->position.y = c.y + (e->position.y - c.y) * f;
                e->position.z = c.z + (e->position.z - c.z) * f;
            }
        }
        return 0;
    }

    if (strcmp(p, "COLOR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        const char *col = arg_o(in, "color", "#ffffff");
        for (int i = 0; i < obj.count; i++)
            strncpy(obj.items[i]->color, col, sizeof(obj.items[i]->color) - 1);
        return 0;
    }

    // ── Comportamientos ──────────────────────────────────────────────────────
    if (strcmp(p, "GIRAR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        char eje = arg_o(in, "eje", "")[0];
        if (eje != 'x' && eje != 'y' && eje != 'z') {
            runtime_error(prog, in, "eje invalido: se espera x, y o z");
            return -1;
        }
        float vel = parse_num(paed_get_arg(in, "velocidad"));
        for (int i = 0; i < obj.count; i++) {
            obj.items[i]->giro_eje       = eje;
            obj.items[i]->giro_velocidad = vel;
        }
        return 0;
    }

    if (strcmp(p, "OSCILAR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        float amp  = parse_num(paed_get_arg(in, "amplitud"));
        float frec = parse_num(paed_get_arg(in, "frecuencia"));
        for (int i = 0; i < obj.count; i++) {
            obj.items[i]->osc_amplitud   = amp;
            obj.items[i]->osc_frecuencia = frec;
        }
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

    for (int i = 0; i < scene->cuerpo_count; i++) {
        const Cuerpo *e = &scene->cuerpos[i];
        printf("  [%s] %-10s pos=(%.1f,%.1f,%.1f)  color=%s",
               e->kind, e->id,
               e->position.x, e->position.y, e->position.z,
               e->color);
        if (e->grupo[0])          printf("  grupo=%s", e->grupo);
        if (e->radio > 0.0f)      printf("  radio=%.2f", e->radio);
        if (e->giro_eje)          printf("  gira en %c a %.2f", e->giro_eje, e->giro_velocidad);
        if (e->osc_frecuencia)    printf("  oscila %.2f/%.2f", e->osc_amplitud, e->osc_frecuencia);
        printf("\n");
    }
}
