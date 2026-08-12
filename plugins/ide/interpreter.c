#include "interpreter.h"
#include "expr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>    // isalpha/isalnum: validar el destino de un LEER
#include <strings.h>  // strcasecmp: los nombres de procedimiento no distinguen mayusculas
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

// ── Entrada de datos (LEER de consola) ───────────────────────────────────────
static PaedEntrada entrada_fn = NULL;
static void       *entrada_ud = NULL;

void interp_set_entrada(PaedEntrada fn, void *ud) {
    entrada_fn = fn;
    entrada_ud = ud;
}

// ¿Sirve como destino de un LEER? Tiene que ser un nombre: 'x', o 'p.campo'.
// Descarta LEER("hola") y LEER(3), que no tienen donde guardar nada.
static int es_destino(const char *s) {
    if (!s || !*s) return 0;
    if (!isalpha((unsigned char)*s) && *s != '_') return 0;
    for (const char *c = s + 1; *c; c++)
        if (!isalnum((unsigned char)*c) && *c != '_' && *c != '.') return 0;
    return 1;
}

// Parte un destino escrito `A[i]` en nombre e indice. Sin corchetes, el indice
// queda vacio. Devuelve 0 si esta bien escrito, -1 si no.
//
// El parser ya hace esto para el destino de `:=` (parser.c:265) y guarda el
// indice como un argumento aparte. Los argumentos de LEER, en cambio, llegan
// crudos porque LEER es variadico: cualquiera de ellos puede ser 'A[i]', no
// solo el primero.
static int partir_destino(const char *destino, char *nombre, size_t nn,
                          char *indice, size_t ni) {
    const char *corchete = strchr(destino, '[');
    if (!corchete) {
        snprintf(nombre, nn, "%s", destino);
        indice[0] = '\0';
        return es_destino(nombre) ? 0 : -1;
    }

    size_t len = strlen(destino);
    if (len == 0 || destino[len - 1] != ']') return -1;

    size_t largo_nombre = (size_t)(corchete - destino);
    if (largo_nombre == 0 || largo_nombre >= nn) return -1;
    snprintf(nombre, nn, "%.*s", (int)largo_nombre, destino);

    size_t largo_indice = len - largo_nombre - 2;   // sin '[' ni ']'
    if (largo_indice == 0 || largo_indice >= ni) return -1;
    snprintf(indice, ni, "%.*s", (int)largo_indice, corchete + 1);

    return es_destino(nombre) ? 0 : -1;
}

// Guarda `v` en un destino ya partido: `nombre` y, si es un elemento de
// arreglo, `idx_txt` con el indice SIN evaluar. Lo comparten la asignacion y
// LEER, que guardan en los mismos tres lugares (escalar, A[i], p.campo) y solo
// se diferencian en de donde sale el valor.
static int guardar_valor(const PAEDProgram *prog, const PAEDInstr *in,
                         const char *nombre, const char *idx_txt, Valor v) {
    if (!idx_txt || !*idx_txt) {
        // Un campo de registro NO nace en su primera asignacion, al reves que
        // un escalar: los campos se crearon todos al declarar la variable. Si
        // el nombre tiene punto y no existe, es un campo que el registro no
        // tiene — y aceptarlo callado dejaria el registro sin sentido, porque
        // admitiria cualquier campo inventado.
        const char *punto = strchr(nombre, '.');
        if (punto && !env_existe(&env, nombre)) {
            char msg[PAED_MSG_MAX];
            snprintf(msg, sizeof(msg),
                     "'%.*s' no tiene un campo '%s'",
                     (int)(punto - nombre), nombre, punto + 1);
            runtime_error(prog, in, msg);
            return -1;
        }

        if (env_set(&env, nombre, v) != 0) {
            // env_set deja el motivo cuando el nombre ya existe como arreglo.
            runtime_error(prog, in, env.error[0] ? env.error : "no entran mas variables");
            return -1;
        }
        return 0;
    }

    // El indice se evalua RECIEN AHORA, no al parsear: en A[i] := 0 dentro de
    // un bucle, i vale distinto en cada vuelta.
    Valor idx;
    if (expr_eval(idx_txt, &env, &idx) != 0) {
        runtime_error(prog, in, env.error);
        return -1;
    }
    if (idx.tipo != VAL_NUM || idx.num != floor(idx.num)) {
        char msg[PAED_MSG_MAX];
        snprintf(msg, sizeof(msg), "el indice de %s tiene que ser un numero entero", nombre);
        runtime_error(prog, in, msg);
        return -1;
    }

    Valor *elem = env_elem(&env, nombre, (int)idx.num);
    if (!elem) {
        runtime_error(prog, in, env.error);
        return -1;
    }
    *elem = v;
    return 0;
}

// Le saca los espacios de los dos lados, EN EL LUGAR. El '\r' cuenta como
// espacio a proposito: un archivo de datos guardado en Windows termina cada
// linea con "\r\n", y ese '\r' invisible convertiria el numero 12 en el texto
// "12\r". El bug seria mudo y la culpa se la llevaria el interprete.
static char *trim_linea(char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n'))
        s[--n] = '\0';
    return s;
}

// Convierte lo que vino por la entrada en un Valor.
//
// PAED declara los tipos en el AMBIENTE, pero el Entorno todavia no los usa
// (esta anotado en el KANBAN), asi que el tipo lo decide EL DATO: si la linea
// entera es un numero, es numero; si no, es texto. Entera, no "empieza con":
// "12abc" es el texto "12abc" y no el numero 12, porque un numero a medias
// escondería el error del que cargo el dato.
static Valor valor_desde_texto(const char *linea) {
    Valor v;
    memset(&v, 0, sizeof(v));

    char *fin = NULL;
    double d  = strtod(linea, &fin);
    if (fin != linea) {
        while (*fin == ' ' || *fin == '\t') fin++;
        if (*fin == '\0') {
            v.tipo = VAL_NUM;
            v.num  = d;
            return v;
        }
    }

    v.tipo = VAL_TEXTO;
    snprintf(v.texto, sizeof(v.texto), "%s", linea);
    return v;
}

// LEER(A, B, p.campo) — pide un dato por destino y lo guarda.
//
// UNA LINEA POR DESTINO, no un token: asi un texto con espacios entra entero.
// Partir por espacios haria que "Juan Perez" llenara dos destinos con medio
// nombre cada uno.
static int leer_consola(const PAEDProgram *prog, const PAEDInstr *in) {
    if (in->arg_count == 0) {
        runtime_error(prog, in, "LEER necesita al menos un destino: LEER(x)");
        return -1;
    }

    if (!entrada_fn) {
        runtime_error(prog, in,
                      "LEER no tiene de donde sacar los datos: este programa corre sin "
                      "entrada enganchada (probalo con build/paedrun)");
        return -1;
    }

    for (int i = 0; i < in->arg_count; i++) {
        const char *destino = in->args[i].val;

        char nombre[PAED_NAME_MAX];
        char indice[PAED_VAL_MAX];
        if (partir_destino(destino, nombre, sizeof(nombre), indice, sizeof(indice)) != 0) {
            char msg[PAED_MSG_MAX];
            snprintf(msg, sizeof(msg),
                     "'%s' no sirve como destino de LEER: se espera una variable, "
                     "un elemento A[i] o un campo p.campo", destino);
            runtime_error(prog, in, msg);
            return -1;
        }

        char linea[PAED_VAL_MAX];
        if (entrada_fn(linea, sizeof(linea), entrada_ud) != 0) {
            char msg[PAED_MSG_MAX];
            snprintf(msg, sizeof(msg),
                     "la entrada se termino antes de darle un valor a '%s'", destino);
            runtime_error(prog, in, msg);
            return -1;
        }

        char *dato = trim_linea(linea);
        if (!*dato) {
            // Una linea vacia casi siempre es un dato que falta, no un texto
            // vacio a proposito. Se avisa en vez de guardar "" callado y que el
            // error aparezca tres instrucciones despues, sin relacion aparente.
            char msg[PAED_MSG_MAX];
            snprintf(msg, sizeof(msg), "la entrada trajo una linea vacia para '%s'", destino);
            runtime_error(prog, in, msg);
            return -1;
        }

        if (guardar_valor(prog, in, nombre, indice, valor_desde_texto(dato)) != 0)
            return -1;
    }

    return 0;
}

static int exec_instr(SceneState *scene, const PAEDProgram *prog, const PAEDInstr *in) {
    const char *p = in->proc;

    // ── Globales ─────────────────────────────────────────────────────────────
    if (strcasecmp(p, "FONDO") == 0) {
        strncpy(scene->bg_color, arg_o(in, "color", "#000000"), sizeof(scene->bg_color) - 1);
        return 0;
    }
    if (strcasecmp(p, "CAMARA") == 0) {
        const char *pos   = paed_get_arg(in, "posicion");
        const char *mirar = paed_get_arg(in, "mirar");
        if (pos)   scene->cam_pos    = parse_vec3(pos);
        if (mirar) scene->cam_target = parse_vec3(mirar);
        return 0;
    }

    // ── Creacion de entidades ────────────────────────────────────────────────
    int es_cubo   = strcasecmp(p, "CUBO")   == 0;
    int es_esfera = strcasecmp(p, "ESFERA") == 0;
    int es_plano  = strcasecmp(p, "PLANO")  == 0;
    int es_luz    = strcasecmp(p, "LUZ")    == 0;

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

    if (strcasecmp(p, "MOVER") == 0) {
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

    if (strcasecmp(p, "ROTAR") == 0) {
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

    if (strcasecmp(p, "ESCALAR") == 0) {
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

    if (strcasecmp(p, "COLOR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        const char *col = arg_o(in, "color", "#ffffff");
        for (int i = 0; i < obj.count; i++)
            strncpy(obj.items[i]->color, col, sizeof(obj.items[i]->color) - 1);
        return 0;
    }

    // ── Comportamientos ──────────────────────────────────────────────────────
    if (strcasecmp(p, "GIRAR") == 0) {
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

    if (strcasecmp(p, "OSCILAR") == 0) {
        if (resolver(scene, prog, in, &obj) != 0) return -1;
        float amp  = parse_num(paed_get_arg(in, "amplitud"));
        float frec = parse_num(paed_get_arg(in, "frecuencia"));
        for (int i = 0; i < obj.count; i++) {
            obj.items[i]->osc_amplitud   = amp;
            obj.items[i]->osc_frecuencia = frec;
        }
        return 0;
    }

    // ── Entrada y salida ─────────────────────────────────────────────────────
    //
    // LEER y ESCRIBIR tienen DOS formas y el parser ya decidio cual es esta
    // (in->forma), mirando si el primer argumento se declaro como archivo.
    // Aca solo se despacha. Que la bifurcacion vaya ANTES del bucle de
    // impresion no es un detalle: ESCRIBIR(arch, reg) entraba al bucle, se
    // evaluaba 'arch' como si fuera una variable y el error que salia era
    // "la variable 'arch' no tiene valor todavia", que manda a mirar al lugar
    // equivocado.
    // ABRIR y CERRAR van PRIMERO: tambien operan sobre un archivo, asi que el
    // parser les pone forma ARCHIVO, y si el chequeo de abajo fuera antes
    // dirian que "no graban" — que no es lo que hacen.
    if (strcasecmp(p, "ABRIR") == 0 || strcasecmp(p, "CERRAR") == 0) {
        char msg[PAED_MSG_MAX];
        snprintf(msg, sizeof(msg),
                 "%s todavia no esta implementado: por ahora solo se valida la declaracion", p);
        runtime_error(prog, in, msg);
        return -1;
    }

    if (in->forma == PAED_FORMA_ARCHIVO) {
        char msg[PAED_MSG_MAX];
        snprintf(msg, sizeof(msg),
                 "%s(archivo, registro) todavia no %s: falta el manejo de archivos en disco",
                 p, strcasecmp(p, "LEER") == 0 ? "avanza el archivo" : "graba");
        runtime_error(prog, in, msg);
        return -1;
    }

    if (strcasecmp(p, "LEER") == 0) return leer_consola(prog, in);

    // ── Salida por consola ───────────────────────────────────────────────────
    if (strcasecmp(p, "ESCRIBIR") == 0) {
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

// Reserva los arreglos declarados en el AMBIENTE. Un escalar no hace falta
// declararlo (nace en su primera asignacion), pero un arreglo si: sin los
// limites no hay donde guardar ni contra que chequear el indice.
// ¿El tipo de esta declaracion es un REGISTRO declarado en el AMBIENTE?
static const PAEDRegistro *registro_de(const PAEDProgram *prog, const char *tipo) {
    for (int i = 0; i < prog->registro_count; i++)
        if (strcasecmp(prog->registros[i].name, tipo) == 0)
            return &prog->registros[i];
    return NULL;
}

static int declarar_ambiente(const PAEDProgram *prog) {
    int fallos = 0;
    for (int i = 0; i < prog->decl_count; i++) {
        const PAEDDecl *d = &prog->decls[i];

        if (d->es_arreglo) {
            if (env_declarar_arreglo(&env, d->name, d->desde, d->hasta) != 0) {
                fprintf(stderr, "%s:%d: error: %s\n", prog->path, d->line, env.error);
                fallos++;
            }
            continue;
        }

        // Variable de tipo REGISTRO: se APLANA en una variable por campo.
        // `pori: vector2` con campos vx,vy pasa a ser "pori.vx" y "pori.vy".
        //
        // Asi el entorno no necesita saber nada de registros: para el son dos
        // variables comunes que da la casualidad que tienen un punto en el
        // nombre. El precio es que no se puede asignar un registro entero
        // (`p1 := p2`), que no aparece en el corpus.
        const PAEDRegistro *reg = registro_de(prog, d->type);
        if (!reg) continue;   // tipo escalar: nace solo en su primera asignacion

        for (int c = 0; c < reg->campo_count; c++) {
            char completo[PAED_NAME_MAX];
            snprintf(completo, sizeof(completo), "%s.%s", d->name, reg->campos[c].name);

            Valor cero = {0};
            cero.tipo = VAL_NUM;
            if (env_set(&env, completo, cero) != 0) {
                fprintf(stderr, "%s:%d: error: no entran mas variables (campo '%s')\n",
                        prog->path, d->line, completo);
                fallos++;
            }
        }
    }
    return fallos;
}

// destino := expr, donde destino puede ser un escalar, un elemento A[i] o un
// campo p.campo. El parser ya partio el destino: el nombre queda en `proc` y el
// indice, si lo hay, como el argumento "indice".
static int asignar(const PAEDProgram *prog, const PAEDInstr *in) {
    Valor v;
    if (expr_eval(in->cond, &env, &v) != 0) {
        runtime_error(prog, in, env.error);
        return -1;
    }
    return guardar_valor(prog, in, in->proc, paed_get_arg(in, "indice"), v);
}

int interp_exec(SceneState *scene, const PAEDProgram *prog) {
    env_init(&env);
    int fallos_ambiente = declarar_ambiente(prog);
    if (fallos_ambiente > 0) return -1;   // sin sus arreglos, el programa no corre

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

            case PAED_ASIGNA:
                if (asignar(prog, in) != 0) fallos++;
                i++;
                break;

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
