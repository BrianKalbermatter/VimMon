#include "interpreter.h"
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>   // sinf/cosf: rotar un grupo alrededor de su centro

// El parser ya valido nombres, tipos y obligatorios contra sintaxis.json.
// Aca solo queda ejecutar: buscar la entidad y aplicar el efecto.

// Las variables del programa. A nivel de archivo y no dentro de interp_exec
// porque ESCRIBIR, que vive en exec_instr, tambien necesita leerlas.
static Entorno env;

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

            if (n >= 2 && v[0] == '"' && v[n - 1] == '"') {
                printf("%.*s", (int)(n - 2), v + 1);   // texto literal, sin comillas
                continue;
            }

            // Lo que no es literal se EVALUA: antes ESCRIBIR(cont_pal)
            // imprimia "cont_pal", el nombre, en vez del valor.
            Valor val;
            char  buf[PAED_VAL_MAX];
            if (expr_eval(v, &env, &val) == 0) {
                valor_a_texto(&val, buf, sizeof(buf));
                printf("%s", buf);
            } else {
                runtime_error(prog, in, env.error);
                return -1;
            }
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

// Un programa mal escrito puede quedar dando vueltas para siempre, y el
// interprete corre DENTRO del game loop: colgarlo cuelga la ventana entera.
// Se corta y se avisa, que es infinitamente mejor que tener que matar el
// proceso sin saber por que.
#define PAED_MAX_PASOS 2000000

// ¿La condicion de un SI/MIENTRAS es verdadera? Deja el motivo en el entorno
// si no se pudo evaluar.
static int condicion(Entorno *env, const PAEDProgram *prog, const PAEDInstr *in, int *ok) {
    Valor v;
    if (expr_eval(in->cond, env, &v) != 0) {
        runtime_error(prog, in, env->error);
        *ok = 0;
        return 0;
    }
    *ok = 1;
    return valor_verdadero(&v);
}

int interp_exec(SceneState *scene, const PAEDProgram *prog) {
    env_init(&env);

    // Un PARA tiene que inicializar su variable la PRIMERA vez que se entra,
    // pero no cada vuelta. Como el FIN_PARA salta de vuelta al PARA, hace
    // falta recordar cual ya arranco. Es por instruccion y no una sola bandera
    // para que dos PARA anidados no se pisen.
    static char para_activo[PAED_MAX_INSTRS];
    memset(para_activo, 0, sizeof(para_activo));

    int fallos = 0;
    int pasos  = 0;
    int i      = 0;

    // Se sigue el hilo con un indice en vez de recorrer el array de punta a
    // punta: el flujo ya no es lineal. Esto es, literalmente, un contador de
    // programa.
    while (i >= 0 && i < prog->instr_count) {
        if (++pasos > PAED_MAX_PASOS) {
            fprintf(stderr, "%s: error: el programa paso los %d pasos sin terminar "
                            "(bucle infinito?)\n", prog->path, PAED_MAX_PASOS);
            fallos++;
            break;
        }

        const PAEDInstr *in = &prog->instrs[i];
        int ok = 1;

        switch (in->kind) {
            case PAED_LLAMADA:
                if (exec_instr(scene, prog, in) != 0) fallos++;
                i++;
                break;

            case PAED_ASIGNA: {
                Valor v;
                if (expr_eval(in->cond, &env, &v) != 0) {
                    runtime_error(prog, in, env.error);
                    fallos++;
                } else if (env_set(&env, in->proc, v) != 0) {
                    runtime_error(prog, in, "no entran mas variables");
                    fallos++;
                }
                i++;
                break;
            }

            case PAED_SI:
                // Verdadera: se sigue derecho al cuerpo. Falsa: al SINO, o a
                // la instruccion de despues del FIN_SI.
                i = condicion(&env, prog, in, &ok) ? i + 1 : in->salto;
                if (!ok) { fallos++; i = in->salto; }
                break;

            case PAED_SINO:
                // Llegar aca EJECUTANDO significa que se termino la rama
                // verdadera, asi que hay que saltearse la rama del SINO.
                i = in->salto;
                break;

            case PAED_FIN_SI:
                i++;
                break;

            case PAED_MIENTRAS:
                i = condicion(&env, prog, in, &ok) ? i + 1 : in->salto;
                if (!ok) { fallos++; i = in->salto; }
                break;

            case PAED_PARA: {
                Valor desde, hasta, paso;
                const char *sd = paed_get_arg(in, "desde");
                const char *sh = paed_get_arg(in, "hasta");
                const char *sp = paed_get_arg(in, "paso");

                if (expr_eval(sd ? sd : "0", &env, &desde) != 0 ||
                    expr_eval(sh ? sh : "0", &env, &hasta) != 0 ||
                    expr_eval(sp ? sp : "1", &env, &paso)  != 0) {
                    runtime_error(prog, in, env.error);
                    fallos++;
                    i = in->salto;
                    break;
                }

                if (!para_activo[i]) {
                    para_activo[i] = 1;
                    env_set(&env, in->proc, desde);
                }

                Valor *v = env_buscar(&env, in->proc);
                // Con paso positivo se corta al pasarse del final; con paso
                // negativo, al bajar de el. Sin esto, un PARA en reversa
                // nunca terminaria.
                int sigue = (paso.num >= 0) ? (v->num <= hasta.num)
                                            : (v->num >= hasta.num);
                if (sigue) {
                    i++;
                } else {
                    // Se apaga la marca: si este PARA esta adentro de otro
                    // bucle, la proxima vuelta tiene que volver a arrancar.
                    para_activo[i] = 0;
                    i = in->salto;
                }
                break;
            }

            case PAED_FIN_PARA: {
                const PAEDInstr *cab = &prog->instrs[in->salto];
                Valor *v = env_buscar(&env, cab->proc);
                Valor  paso;
                const char *sp = paed_get_arg(cab, "paso");
                if (v && expr_eval(sp ? sp : "1", &env, &paso) == 0)
                    v->num += paso.num;
                i = in->salto;   // volver al PARA a re-chequear
                break;
            }

            case PAED_FIN_MIENTRAS:
                i = in->salto;   // volver a evaluar la condicion
                break;
        }

        if (!ok && fallos > PAED_MAX_ERRORS) break;
    }

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
