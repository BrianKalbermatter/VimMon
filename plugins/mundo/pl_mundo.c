// ============================================================
// Los once verbos del juego. Ver pl_mundo.h para el panorama.
//
// ── COMO SE ESCRIBE UNA FUNCION ─────────────────────────────
// Firma fija (interpreter.h):
//
//   int nombre(const char *const *args, int n, Valor *out,
//              char *error, size_t error_n, void *ud)
//
//   args   el TEXTO de cada argumento, sin evaluar
//   n      cuantos argumentos vinieron
//   out    donde dejas el resultado
//   error  si algo sale mal, el motivo va aca y devolves -1
//
// Para resolver el texto de un argumento se usa paed_eval(), NUNCA expr_eval:
// el Entorno con las variables del programa es privado del interprete.
//
// Devolver un numero:  *out = (Valor){ .tipo = VAL_NUM, .num = 42 };
//
// ── COMO SE ESCRIBE UN PROCEDIMIENTO ────────────────────────
// Firma fija:
//
//   int nombre(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
//
// Los argumentos vienen CON NOMBRE, se piden por clave:
//
//   const char *s = paed_get_arg(in, "color");
//
// Si algo esta mal:  paed_runtime_error(prog, in, "..."); return -1;
// ============================================================

#include "pl_mundo.h"

#include "paed/interpreter.h"
#include "paed/parser.h"
#include "paed/expr.h"

#include "../renderer3d/renderer3d.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// El backend concreto. Un solo renderer para todo el proceso: el juego tiene
// una ventana, no varias.
static Renderer3D *R = &gpu_sdl_renderer3d;

// La camara del frame que se esta armando. FRAME_INICIO la llena y
// dibujar_malla la usa por debajo; los verbos de dibujo no la tocan.
static Camara3D g_cam;

// Movimiento del mouse del frame en curso.
//
// R->mouse_delta() CONSUME lo que devuelve, asi que si MOUSE_X y MOUSE_Y lo
// llamaran cada una por su lado, la segunda recibiria cero. Se lee UNA vez por
// frame —adentro de SALIR(), que es el pulso del game loop— y las dos funciones
// leen de aca.
static float g_mdx = 0.0f, g_mdy = 0.0f;

// Mallas y texturas se crean UNA vez, no por frame: crear un buffer de GPU en
// cada cuadro es la forma mas facil de tirar los 60fps a la basura.
static Malla   g_cubo    = MALLA_NULA;
static Textura g_blanca  = TEXTURA_NULA;

// ── Ayudas ──────────────────────────────────────────────────

// Evalua el TEXTO de un argumento y lo devuelve como numero.
// Los argumentos llegan sin evaluar, asi que 'x + 1' o 'px' funcionan igual
// que '3'. Por eso no alcanza con atof.
static double num_de(const char *texto, int *ok)
{
    Valor v;
    char  err[PAED_MSG_MAX] = {0};
    if (!texto || paed_eval(texto, &v, err, sizeof(err)) != 0) {
        if (ok) *ok = 0;
        return 0.0;
    }
    if (ok) *ok = 1;
    return (v.tipo == VAL_NUM) ? v.num : (double)v.logico;
}

// Un argumento con nombre, leido como numero. Si no vino, devuelve `porDefecto`.
static double arg_num(const PAEDInstr *in, const char *clave, double porDefecto)
{
    const char *s = paed_get_arg(in, clave);
    if (!s || !*s) return porDefecto;
    int ok = 0;
    double v = num_de(s, &ok);
    return ok ? v : porDefecto;
}

// Traduce un nombre de color a ARGB. Los colores tienen nombre y no numero
// hexadecimal a proposito: quien escribe el .paed esta aprendiendo a programar,
// no a leer 0xFF8C8C96.
//
// Devuelve 0 si no conoce el nombre.
static int color_de(const char *texto, uint32_t *out)
{
    // Los argumentos llegan con las comillas puestas: 'gris', no gris.
    static const struct { const char *nombre; uint32_t argb; } tabla[] = {
        { "'gris'",   RGB3(140, 140, 150) },
        { "'rojo'",   RGB3(190,  60,  50) },
        { "'verde'",  RGB3( 70, 170,  80) },
        { "'azul'",   RGB3( 60,  90, 190) },
        { "'amarillo'", RGB3(220, 190,  70) },
        { "'negro'",  RGB3( 15,  15,  20) },
        { "'blanco'", BLANCO },
    };
    for (size_t i = 0; i < sizeof(tabla) / sizeof(tabla[0]); i++)
        if (strcasecmp(texto, tabla[i].nombre) == 0) { *out = tabla[i].argb; return 1; }
    return 0;
}

// Un argumento de color con nombre. Si no vino, devuelve `porDefecto`.
// Si vino pero no se conoce, avisa y devuelve 0 para que el verbo corte.
static int arg_color(const PAEDProgram *prog, const PAEDInstr *in,
                     const char *clave, uint32_t porDefecto, uint32_t *out)
{
    const char *s = paed_get_arg(in, clave);
    if (!s || !*s) { *out = porDefecto; return 1; }
    if (!color_de(s, out)) {
        paed_runtime_error(prog, in,
            "color desconocido: gris, rojo, verde, azul, amarillo, negro o blanco");
        return 0;
    }
    return 1;
}

// Deja un numero en `out`. Es el ultimo paso de toda funcion que devuelve algo.
static void devolver_num(Valor *out, double n)
{
    memset(out, 0, sizeof(*out));
    out->tipo = VAL_NUM;
    out->num  = n;
}

// ── FUNCIONES ───────────────────────────────────────────────

// TECLA('W') -> 1 si esta apretada, 0 si no.
//
// EJEMPLO COMPLETO: el resto de las funciones son variaciones de esta.
static int fn_tecla(const char *const *args, int n, Valor *out,
                    char *error, size_t error_n, void *ud)
{
    (void)ud;

    if (n != 1) {
        snprintf(error, error_n, "TECLA lleva UNA tecla: TECLA('W')");
        return -1;
    }

    // El argumento llega como texto sin evaluar: puede ser 'W' (con comillas,
    // que es como se escribe un caracter en AED) o una variable que contiene
    // una letra. Se evalua para cubrir los dos casos.
    Valor v;
    char  err[PAED_MSG_MAX] = {0};
    if (paed_eval(args[0], &v, err, sizeof(err)) != 0) {
        snprintf(error, error_n, "TECLA: %s", err);
        return -1;
    }
    if (v.tipo != VAL_TEXTO || !v.texto[0]) {
        snprintf(error, error_n, "TECLA espera una letra entre comillas: TECLA('W')");
        return -1;
    }

    // De la letra que escribio el programador al enum del renderer.
    // La tabla es explicita a proposito: si manana el renderer agrega teclas,
    // el compilador no avisa nada, pero esta tabla se lee de un vistazo.
    Tecla3D k;
    switch (v.texto[0]) {
        case 'w': case 'W': k = T3_W;      break;
        case 'a': case 'A': k = T3_A;      break;
        case 's': case 'S': k = T3_S;      break;
        case 'd': case 'D': k = T3_D;      break;
        case ' ':           k = T3_SPACE;  break;
        default:
            snprintf(error, error_n,
                     "TECLA no conoce '%c': por ahora W, A, S, D y el espacio",
                     v.texto[0]);
            return -1;
    }

    devolver_num(out, R->tecla(k) ? 1 : 0);
    return 0;
}

// SALIR() -> 1 si cerraron la ventana.
//
// Ademas de preguntar, esta funcion es el PULSO del juego: por debajo vacia la
// cola de eventos de SDL, y esa cola es donde vive el movimiento del mouse. Por
// eso se llama una vez por frame aunque no te importe si cerraron la ventana:
// si no, el mouse se siente trabado.
static int fn_salir(const char *const *args, int n, Valor *out,
                    char *error, size_t error_n, void *ud)
{
    (void)args; (void)ud;

    if (n != 0) {
        snprintf(error, error_n, "SALIR no lleva argumentos: SALIR()");
        return -1;
    }

    int cerraron = R->poll_quit();

    // Recien despues de vaciar la cola hay movimiento para leer, y se lee aca
    // porque leerlo lo consume. MOUSE_X y MOUSE_Y despues solo miran.
    R->mouse_delta(&g_mdx, &g_mdy);

    devolver_num(out, cerraron ? 1 : 0);
    return 0;
}

// MOUSE_X() -> cuanto se movio el mouse en horizontal desde el frame pasado.
// Negativo hacia la izquierda, positivo hacia la derecha, 0 si no se movio.
static int fn_mouse_x(const char *const *args, int n, Valor *out,
                      char *error, size_t error_n, void *ud)
{
    (void)args; (void)ud;
    if (n != 0) { snprintf(error, error_n, "MOUSE_X no lleva argumentos"); return -1; }
    devolver_num(out, g_mdx);
    return 0;
}

// MOUSE_Y() -> idem en vertical. Positivo hacia abajo, que es como cuenta la
// pantalla: el (0,0) esta arriba a la izquierda, no abajo.
static int fn_mouse_y(const char *const *args, int n, Valor *out,
                      char *error, size_t error_n, void *ud)
{
    (void)args; (void)ud;
    if (n != 0) { snprintf(error, error_n, "MOUSE_Y no lleva argumentos"); return -1; }
    devolver_num(out, g_mdy);
    return 0;
}

// CLIC('izq') -> 1 si ese boton esta apretado.
static int fn_clic(const char *const *args, int n, Valor *out,
                   char *error, size_t error_n, void *ud)
{
    (void)ud;

    if (n != 1) {
        snprintf(error, error_n, "CLIC lleva un boton: CLIC('izq')");
        return -1;
    }

    Valor v;
    char  err[PAED_MSG_MAX] = {0};
    if (paed_eval(args[0], &v, err, sizeof(err)) != 0) {
        snprintf(error, error_n, "CLIC: %s", err);
        return -1;
    }
    if (v.tipo != VAL_TEXTO) {
        snprintf(error, error_n, "CLIC espera 'izq', 'der' o 'medio' entre comillas");
        return -1;
    }

    Boton3D b;
    if      (strcasecmp(v.texto, "izq")   == 0) b = M3_IZQ;
    else if (strcasecmp(v.texto, "der")   == 0) b = M3_DER;
    else if (strcasecmp(v.texto, "medio") == 0) b = M3_MEDIO;
    else {
        snprintf(error, error_n, "CLIC no conoce '%s': izq, der o medio", v.texto);
        return -1;
    }

    devolver_num(out, R->mouse_boton(b) ? 1 : 0);
    return 0;
}

// TICKS() -> milisegundos desde que arranco el programa.
//
// Sirve para que el juego vaya igual de rapido en cualquier maquina: en vez de
// mover "3 por vuelta", moves "3 por segundo" y calculas cuanto paso.
static int fn_ticks(const char *const *args, int n, Valor *out,
                    char *error, size_t error_n, void *ud)
{
    (void)args; (void)ud;
    if (n != 0) { snprintf(error, error_n, "TICKS no lleva argumentos"); return -1; }
    devolver_num(out, (double)R->ticks_ms());
    return 0;
}

// ── PROCEDIMIENTOS ──────────────────────────────────────────

// CUBO(x = 3, y = 0, z = 5, color = 'gris')
//
// EJEMPLO COMPLETO: el resto de los procedimientos son variaciones de este.
static int proc_cubo(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
{
    (void)ud;

    if (g_cubo == MALLA_NULA) {
        paed_runtime_error(prog, in, "falta INICIAR antes de dibujar");
        return -1;
    }

    V3 pos = { (float)arg_num(in, "x", 0.0),
               (float)arg_num(in, "y", 0.0),
               (float)arg_num(in, "z", 0.0) };

    V3 rot = { 0.0f, 0.0f, 0.0f };   // un cubo del laberinto no rota
    V3 esc = { 1.0f, 1.0f, 1.0f };   // una celda mide 1

    // El tinte multiplica la textura. Con la textura blanca de arriba, el tinte
    // ES el color: blanco x color = color. Un solo cubo sirve para todo el
    // laberinto y se pinta distinto en cada llamada, sin crear nada nuevo.
    uint32_t tinte;
    if (!arg_color(prog, in, "color", BLANCO, &tinte)) return -1;

    R->dibujar_malla(g_cubo, pos, rot, esc, g_blanca, tinte);
    return 0;
}

// INICIAR(titulo = 'Laberinto', ancho = 960, alto = 540)
//
// Abre la ventana y crea de una vez los recursos de GPU. El lienzo INTERNO sale
// a la mitad de la ventana: es lo que da el pixel art parejo (ver renderer3d.h).
static int proc_iniciar(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
{
    (void)ud;

    if (g_cubo != MALLA_NULA) {
        paed_runtime_error(prog, in, "INICIAR ya se llamo: va una sola vez");
        return -1;
    }

    const char *titulo = paed_get_arg(in, "titulo");
    char limpio[128] = "PAED";
    if (titulo && *titulo) {
        // Llega con las comillas del pseudocodigo puestas; se las saca.
        size_t largo = strlen(titulo);
        if (largo >= 2 && (titulo[0] == '\'' || titulo[0] == '"')) {
            largo -= 2; titulo++;
        }
        if (largo >= sizeof(limpio)) largo = sizeof(limpio) - 1;
        memcpy(limpio, titulo, largo);
        limpio[largo] = '\0';
    }

    int ancho = (int)arg_num(in, "ancho", 960);
    int alto  = (int)arg_num(in, "alto",  540);
    if (ancho < 64 || alto < 64) {
        paed_runtime_error(prog, in, "la ventana tiene que medir al menos 64x64");
        return -1;
    }

    // OJO con el sentido: init devuelve 0 cuando SALE BIEN, como todo lo que
    // sigue la convencion de Unix. Leerlo como un booleano lo invierte.
    if (R->init(limpio, ancho, alto, ancho / 2, alto / 2) != 0) {
        paed_runtime_error(prog, in, "no pude abrir la ventana: falta un driver Vulkan?");
        return -1;
    }

    // UNA vez, no por frame. Un cubo y un pixel blanco alcanzan para todo: el
    // color sale del tinte en cada dibujada.
    g_cubo   = R->malla_cubo();
    g_blanca = R->textura_solida(BLANCO);
    return 0;
}

// CAPTURAR_MOUSE(activar = 1)
static int proc_capturar(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
{
    (void)ud;
    if (g_cubo == MALLA_NULA) {
        paed_runtime_error(prog, in, "falta INICIAR antes de capturar el mouse");
        return -1;
    }
    R->mouse_capturar((int)arg_num(in, "activar", 1) != 0);
    return 0;
}

// FRAME_INICIO(camx = 2, camy = 0.5, camz = 3, angulo = 90, cielo = 'negro')
//
// El motor quiere un PUNTO al que mirar; vos le das un ANGULO, que es como se
// piensa con un mouse. La cuenta de abajo es la que traduce una cosa en la otra.
static int proc_frame_inicio(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
{
    (void)ud;

    if (g_cubo == MALLA_NULA) {
        paed_runtime_error(prog, in, "falta INICIAR antes de dibujar");
        return -1;
    }

    double gx = arg_num(in, "camx", 0.0);
    double gy = arg_num(in, "camy", 0.5);
    double gz = arg_num(in, "camz", 0.0);
    double ang_grados = arg_num(in, "angulo", 0.0);

    uint32_t cielo;
    if (!arg_color(prog, in, "cielo", RGB3(20, 20, 30), &cielo)) return -1;

    // sin() y cos() trabajan en RADIANES. Un grado son PI/180 radianes.
    // Sin esta conversion la camara apunta a cualquier lado y no da error:
    // simplemente mira mal, que es mucho peor de encontrar.
    double rad = ang_grados * (M_PI / 180.0);

    g_cam.pos    = (V3){ (float)gx, (float)gy, (float)gz };
    g_cam.mira_a = (V3){ (float)(gx + cos(rad)),
                         (float)gy,
                         (float)(gz + sin(rad)) };
    g_cam.fov_grados = 70.0f;
    g_cam.cerca      = 0.05f;
    g_cam.lejos      = 100.0f;

    R->frame_begin(&g_cam, cielo);
    return 0;
}

// BILLBOARD(x = 5, y = 0.5, z = 3, ancho = 1, alto = 2, color = 'rojo')
//
// Un sprite que SIEMPRE encara a la camara: gires como gires, lo ves de frente.
// Asi dibujaban los enemigos DOOM y Duke Nukem, y por eso no lleva rotacion.
static int proc_billboard(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
{
    (void)ud;

    if (g_cubo == MALLA_NULA) {
        paed_runtime_error(prog, in, "falta INICIAR antes de dibujar");
        return -1;
    }

    V3 pos = { (float)arg_num(in, "x", 0.0),
               (float)arg_num(in, "y", 0.5),
               (float)arg_num(in, "z", 0.0) };

    uint32_t tinte;
    if (!arg_color(prog, in, "color", BLANCO, &tinte)) return -1;

    R->dibujar_billboard(g_blanca, pos,
                         (float)arg_num(in, "ancho", 1.0),
                         (float)arg_num(in, "alto",  1.0),
                         tinte);
    return 0;
}

// FRAME_FIN() — cierra el cuadro, lo escala y lo muestra.
static int proc_frame_fin(const PAEDProgram *prog, const PAEDInstr *in, void *ud)
{
    (void)ud;
    if (g_cubo == MALLA_NULA) {
        paed_runtime_error(prog, in, "falta INICIAR antes de cerrar un cuadro");
        return -1;
    }
    R->frame_end();
    return 0;
}

// ── Enganche ────────────────────────────────────────────────
//
// Aca se anota cada verbo. Descomenta las lineas a medida que escribas cada uno:
// registrar algo que todavia no existe no compila, y esa es justamente la idea
// — el compilador te lleva de la mano.
void mundo_apagar(void)
{
    if (g_cubo == MALLA_NULA) return;   // INICIAR nunca corrio: no hay nada que apagar
    R->shutdown();
    g_cubo   = MALLA_NULA;
    g_blanca = TEXTURA_NULA;
}

int mundo_registrar(void)
{
    int rc = 0;

    // Funciones: no necesitan .json, expr.c las resuelve en runtime.
    rc |= paed_register_func("SALIR",   fn_salir,   NULL);
    rc |= paed_register_func("TECLA",   fn_tecla,   NULL);
    rc |= paed_register_func("MOUSE_X", fn_mouse_x, NULL);
    rc |= paed_register_func("MOUSE_Y", fn_mouse_y, NULL);
    rc |= paed_register_func("CLIC",    fn_clic,    NULL);
    rc |= paed_register_func("TICKS",   fn_ticks,   NULL);

    // Procedimientos: ADEMAS de esto tienen que estar en paed/data/mundo.json,
    // o el parser rechaza el nombre antes de que este codigo llegue a correr.
    rc |= paed_register_proc("INICIAR",        proc_iniciar,      NULL);
    rc |= paed_register_proc("CAPTURAR_MOUSE", proc_capturar,     NULL);
    rc |= paed_register_proc("FRAME_INICIO",   proc_frame_inicio, NULL);
    rc |= paed_register_proc("CUBO",           proc_cubo,         NULL);
    rc |= paed_register_proc("BILLBOARD",      proc_billboard,    NULL);
    rc |= paed_register_proc("FRAME_FIN",      proc_frame_fin,    NULL);

    return rc;
}
