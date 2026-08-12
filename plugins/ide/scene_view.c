#include "scene_view.h"
#include "parser.h"
#include "interpreter.h"
#include "escena.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Cada cuantos frames se mira el mtime. A ~60fps esto es medio segundo, el
// mismo ritmo que usa hot-Reload/host.c. Mirar el disco en CADA frame seria
// 60 syscalls por segundo para detectar algo que cambia cuando guardas.
#define RECARGA_CADA_FRAMES 30

// Campo de vision vertical. 60 grados es lo tipico: mas abierto deforma los
// bordes, mas cerrado se siente como mirar por un tubo.
#define FOV_GRADOS 60.0f

typedef struct {
    SceneState escena;
    char       path[PAED_PATH_MAX];
    long long  mtime;
    int        frames;
} PaedView;

// Una sola vista estatica, igual que g_world en engine.c: sin malloc, listo
// para el dia que no haya heap (FASE 6, bare metal).
static PaedView g_view;

// ── Vectores ──────────────────────────────────────────────────
// Lo minimo para armar una camara. Nada de esto es especifico de PAED.

static Vec3  v_sub  (Vec3 a, Vec3 b) { return (Vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static float v_dot  (Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static float v_largo(Vec3 a)         { return sqrtf(v_dot(a, a)); }

// Producto vectorial: da un vector PERPENDICULAR a los dos. Es lo que permite
// sacar el "derecha" de la camara a partir de hacia donde mira y donde esta
// arriba en el mundo.
static Vec3 v_cruz(Vec3 a, Vec3 b) {
    return (Vec3){ a.y*b.z - a.z*b.y,
                   a.z*b.x - a.x*b.z,
                   a.x*b.y - a.y*b.x };
}

static Vec3 v_norm(Vec3 a) {
    float l = v_largo(a);
    if (l < 1e-6f) return (Vec3){0, 0, 0};
    return (Vec3){a.x/l, a.y/l, a.z/l};
}

// Gira un offset alrededor del origen. Se usa para las esquinas de un cubo:
// asi ROTAR se ve de verdad y no queda como un dato que nadie mira.
static Vec3 v_rotar(Vec3 p, Vec3 grados) {
    const float K = 3.14159265f / 180.0f;
    float rx = grados.x*K, ry = grados.y*K, rz = grados.z*K;
    float s, c, a, b;

    s = sinf(rx); c = cosf(rx);  a = p.y; b = p.z;  p.y = a*c - b*s;  p.z = a*s + b*c;
    s = sinf(ry); c = cosf(ry);  a = p.x; b = p.z;  p.x = a*c + b*s;  p.z = -a*s + b*c;
    s = sinf(rz); c = cosf(rz);  a = p.x; b = p.y;  p.x = a*c - b*s;  p.y = a*s + b*c;
    return p;
}

// ── La camara ─────────────────────────────────────────────────
// Tres vectores perpendiculares entre si (derecha, arriba, adelante) mas una
// distancia focal. Con eso alcanza para pasar de mundo a pantalla.

typedef struct {
    Vec3  der, arr, ade;
    float foco;      // en pixeles
    int   cx, cy;    // centro de la pantalla
} Camara;

static Camara camara_de(const SceneState *s, const Renderer *r) {
    Camara c;

    Vec3 ade = v_sub(s->cam_target, s->cam_pos);
    // Sin CAMARA en el .paed, cam_pos y cam_target quedan los dos en cero y
    // la resta da el vector nulo, que no tiene direccion. Miramos al -z.
    if (v_largo(ade) < 1e-4f) ade = (Vec3){0, 0, -1};
    c.ade = v_norm(ade);

    Vec3 arriba_mundo = {0, 1, 0};
    // Si la camara mira casi en vertical, "adelante" y "arriba" son paralelos
    // y el producto vectorial da cero: no habria un "derecha". Se cambia la
    // referencia para que el cruce vuelva a tener sentido.
    if (fabsf(v_dot(c.ade, arriba_mundo)) > 0.999f) arriba_mundo = (Vec3){0, 0, 1};

    c.der = v_norm(v_cruz(c.ade, arriba_mundo));
    c.arr = v_cruz(c.der, c.ade);

    int alto = r->height();
    c.cx = r->width() / 2;
    c.cy = alto / 2;
    c.foco = (float)alto * 0.5f / tanf(FOV_GRADOS * 0.5f * 3.14159265f / 180.0f);
    return c;
}

// El unico paso realmente "3D" de todo esto: DIVIDIR POR LA PROFUNDIDAD.
// Lo lejano se achica porque z es mas grande. Eso es la perspectiva, entera.
// Devuelve 0 si el punto quedo detras de la camara (no se dibuja).
static int proyectar(const Camara *c, const SceneState *s, Vec3 p,
                     int *sx, int *sy, float *prof) {
    Vec3  v = v_sub(p, s->cam_pos);
    float x = v_dot(v, c->der);   // cuanto a la derecha
    float y = v_dot(v, c->arr);   // cuanto hacia arriba
    float z = v_dot(v, c->ade);   // cuanto adelante = profundidad

    if (z < 0.05f) return 0;      // detras nuestro, o justo encima del ojo

    *prof = z;
    *sx = c->cx + (int)(x * c->foco / z);
    *sy = c->cy - (int)(y * c->foco / z);  // la y de pantalla crece hacia ABAJO
    return 1;
}

// ── Primitivas ────────────────────────────────────────────────

static uint32_t color_de(const char *hex) {
    if (!hex || hex[0] != '#') return RGB(255, 255, 255);
    unsigned long v = strtoul(hex + 1, NULL, 16);
    return RGB((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

// Circulo relleno por barridos horizontales. Una fill_rect de alto 1 por fila
// en vez de put_pixel uno por uno: el backend hace menos llamadas.
static void circulo(const Renderer *r, int cx, int cy, int rad, uint32_t col) {
    if (rad < 1) rad = 1;
    for (int dy = -rad; dy <= rad; dy++) {
        int dx = (int)sqrtf((float)(rad*rad - dy*dy));
        r->fill_rect(cx - dx, cy + dy, dx*2 + 1, 1, col);
    }
}

// Caja: se proyectan las 8 esquinas (ya rotadas) y se rellena el rectangulo
// que las contiene. No es un cubo sombreado, pero el tamanio en pantalla sale
// de la perspectiva real y ROTAR se nota.
static void caja(const Renderer *r, const Camara *c, const SceneState *s,
                 const Cuerpo *e, uint32_t col) {
    float hx = e->scale.x * 0.5f, hy = e->scale.y * 0.5f, hz = e->scale.z * 0.5f;
    int   x0 = 0, y0 = 0, x1 = 0, y1 = 0, hay = 0;

    for (int i = 0; i < 8; i++) {
        // Los bits de i eligen el signo de cada eje: 8 combinaciones = 8 esquinas.
        Vec3 off = { (i & 1) ? hx : -hx,
                     (i & 2) ? hy : -hy,
                     (i & 4) ? hz : -hz };
        off = v_rotar(off, e->rotation);

        Vec3 esquina = { e->position.x + off.x,
                         e->position.y + off.y,
                         e->position.z + off.z };

        int sx, sy; float prof;
        if (!proyectar(c, s, esquina, &sx, &sy, &prof)) continue;

        // EL PROBLEMA DEL PLANO CERCANO. Un objeto grande (el suelo) puede
        // tener esquinas DETRAS de la camara y otras apenas adelante. Las de
        // atras se descartan arriba, pero las que quedan a profundidad casi
        // cero se dividen por casi nada y escupen coordenadas de decenas de
        // miles: la caja resultante deja de significar algo.
        //
        // Un renderer de verdad RECORTA el poligono contra el plano cercano y
        // genera vertices nuevos justo en el borde. Eso todavia no lo hacemos.
        // Mientras tanto se acota a unas pocas pantallas: lo que tenia que
        // tapar la pantalla la sigue tapando (fill_rect recorta al lienzo),
        // pero sin numeros absurdos dando vueltas.
        int lim_x = c->cx * 4, lim_y = c->cy * 4;
        if (sx < -lim_x) sx = -lim_x;  if (sx > lim_x) sx = lim_x;
        if (sy < -lim_y) sy = -lim_y;  if (sy > lim_y) sy = lim_y;

        if (!hay) { x0 = x1 = sx; y0 = y1 = sy; hay = 1; continue; }
        if (sx < x0) x0 = sx;
        if (sx > x1) x1 = sx;
        if (sy < y0) y0 = sy;
        if (sy > y1) y1 = sy;
    }

    if (hay) r->fill_rect(x0, y0, (x1 - x0) + 1, (y1 - y0) + 1, col);
}

// ── Recarga ───────────────────────────────────────────────────

// Con NANOsegundos, no con st_mtime a secas. st_mtime cuenta segundos enteros:
// si guardas dos veces dentro del mismo segundo, la segunda no se nota y te
// quedas mirando una escena vieja preguntandote por que no pasa nada. Guardar
// dos veces seguidas mientras editas es de lo mas comun.
static long long mtime_de(const char *p) {
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    return (long long)st.st_mtim.tv_sec * 1000000000LL + st.st_mtim.tv_nsec;
}

static void recargar(PaedView *v) {
    // static y no local: PAEDProgram lleva 256 instrucciones con sus argumentos
    // y pesa cientos de KB. En la pila, dentro del game loop, es pedir un
    // desborde. En estatico se reserva una sola vez.
    static PAEDProgram prog;

    if (paed_parse_file(v->path, &prog) != 0) {
        paed_print_errors(&prog);
        printf("[view] %d error(es) en %s — sigo mostrando la escena anterior\n",
               prog.error_count, v->path);
        fflush(stdout);
        return;
    }

    // La escena nueva se arma aparte y recien al final reemplaza a la vieja,
    // para que un .paed con errores no deje media escena a la vista. Hay que
    // REGISTRAR apuntando a esta: el registro guarda el puntero, y `nueva` vive
    // en la pila de esta funcion.
    SceneState nueva;
    escena_init(&nueva);
    escena_registrar(&nueva);
    if (interp_exec(&prog) != 0)
        printf("[view] errores de ejecucion: la escena puede quedar incompleta\n");

    v->escena = nueva;

    // `nueva` muere al salir de esta funcion, y el registro se quedo con su
    // direccion. Se reapunta a la escena que SI sobrevive: si no, el proximo
    // que ejecute sin registrar antes escribiria en una pila que ya no existe.
    escena_registrar(&v->escena);

    printf("[view] escena recargada — %d instrucciones, %d cuerpos\n",
           prog.instr_count, nueva.cuerpo_count);
    fflush(stdout);
}

// ── Los dos callbacks que el motor ya sabia llamar ────────────

static void view_update(Entity *self, World *w, float dt) {
    (void)dt;
    PaedView *v = (PaedView *)self->state;

    // El fondo lo manda FONDO(color = ...) del .paed, no el motor.
    w->clear_color = color_de(v->escena.bg_color);

    if (++v->frames < RECARGA_CADA_FRAMES) return;
    v->frames = 0;

    long ahora = mtime_de(v->path);
    if (ahora != 0 && ahora != v->mtime) {
        v->mtime = ahora;
        recargar(v);
    }
}

static void view_draw(Entity *self, const Renderer *r) {
    PaedView         *v = (PaedView *)self->state;
    const SceneState *s = &v->escena;
    Camara            c = camara_de(s, r);

    // Orden del pintor. Sin z-buffer, el ORDEN DE DIBUJO es la profundidad:
    // lo que se pinta despues tapa lo anterior. Si dibujaramos en el orden en
    // que estan declarados en el .paed, un objeto de atras podria tapar uno de
    // adelante. Se ordena de lejos a cerca y se pinta en ese orden.
    typedef struct { int idx, sx, sy; float prof; } Pieza;
    Pieza lista[SCENE_MAX_CUERPOS];
    int   n = 0;

    for (int i = 0; i < s->cuerpo_count; i++) {
        int sx, sy; float prof;
        if (!proyectar(&c, s, s->cuerpos[i].position, &sx, &sy, &prof)) continue;
        lista[n].idx = i; lista[n].sx = sx; lista[n].sy = sy; lista[n].prof = prof;
        n++;
    }

    // Insercion: n como mucho son 64 piezas y el codigo se lee de un vistazo.
    // Traer quicksort aca seria optimizar lo que no molesta.
    for (int a = 1; a < n; a++) {
        Pieza k = lista[a];
        int   b = a - 1;
        while (b >= 0 && lista[b].prof < k.prof) { lista[b+1] = lista[b]; b--; }
        lista[b+1] = k;
    }

    for (int j = 0; j < n; j++) {
        const Cuerpo *e   = &s->cuerpos[lista[j].idx];
        uint32_t      col = color_de(e->color);
        float         esc = c.foco / lista[j].prof;   // pixeles por unidad de mundo

        if (strcmp(e->kind, "esfera") == 0) {
            circulo(r, lista[j].sx, lista[j].sy, (int)(e->radio * esc), col);

        } else if (strcmp(e->kind, "luz") == 0) {
            // Una luz no es geometria: no ilumina nada todavia. Se marca con
            // una cruz para saber donde esta, sin fingir que es un objeto.
            int m = 8;
            r->draw_line(lista[j].sx - m, lista[j].sy, lista[j].sx + m, lista[j].sy, col);
            r->draw_line(lista[j].sx, lista[j].sy - m, lista[j].sx, lista[j].sy + m, col);

        } else {
            caja(r, &c, s, e, col);   // cubo y plano
        }
    }
}

// ── Montaje ───────────────────────────────────────────────────

int scene_view_mount(World *w, const char *paed_path) {
    if (w == NULL || paed_path == NULL) return -1;

    memset(&g_view, 0, sizeof(g_view));
    snprintf(g_view.path, sizeof(g_view.path), "%s", paed_path);
    escena_init(&g_view.escena);

    g_view.mtime = mtime_de(g_view.path);
    recargar(&g_view);

    // UNA entidad para toda la escena PAED. El motor no se entera de nada:
    // ve una entidad mas con su update y su draw, como cualquier otra.
    Entity *e = world_spawn(w);
    if (e == NULL) return -1;

    e->state  = &g_view;
    e->update = view_update;
    e->draw   = view_draw;

    printf("[view] vigilando %s (guardá el archivo y se redibuja solo)\n",
           g_view.path);
    return 0;
}
