# Hot Reload en C — Teoría y práctica

Recarga de código en tiempo de ejecución usando bibliotecas dinámicas.
Notas basadas en la implementación de `dungeon` (`host.c` + `game.c` + `game.h`).

---

## Índice

1. [El problema](#1-el-problema)
2. [Enlazado: estático vs dinámico](#2-enlazado-estático-vs-dinámico)
3. [Qué es realmente un `.so`](#3-qué-es-realmente-un-so)
4. [La API de carga dinámica](#4-la-api-de-carga-dinámica)
5. [Punteros a función: el interruptor](#5-punteros-a-función-el-interruptor)
6. [La separación host / lógica](#6-la-separación-host--lógica)
7. [El estado: la regla central](#7-el-estado-la-regla-central)
8. [Qué sobrevive y qué no](#8-qué-sobrevive-y-qué-no)
9. [Layout de structs y compatibilidad binaria](#9-layout-de-structs-y-compatibilidad-binaria)
10. [Detección de cambios](#10-detección-de-cambios)
11. [El problema del archivo en uso](#11-el-problema-del-archivo-en-uso)
12. [El punto de recarga seguro](#12-el-punto-de-recarga-seguro)
13. [Catálogo de errores](#13-catálogo-de-errores)
14. [Herramientas de diagnóstico](#14-herramientas-de-diagnóstico)
15. [Limitaciones y alternativas](#15-limitaciones-y-alternativas)
16. [Checklist](#16-checklist)
17. [Recorrido por el código de `dungeon`](#17-recorrido-por-el-código-de-dungeon)
18. [Referencias](#18-referencias)

---

## 1. El problema

El ciclo normal de desarrollo en C es:

```
editar → compilar → ejecutar → llegar al estado que se quiere probar → observar
```

El cuarto paso es el costoso. En un juego significa volver a jugar hasta el punto
donde está el bug. Si se está ajustando el balance del combate en la sala final,
cada iteración cuesta minutos de juego para probar un cambio de un número.

El hot reload elimina ese paso:

```
editar → compilar → observar (el programa nunca murió)
```

El proceso sigue vivo, con su memoria intacta, y solo se reemplaza el código.

**Costo:** una arquitectura más estricta y un conjunto de reglas que hay que
respetar. Esas reglas son el contenido de este documento.

---

## 2. Enlazado: estático vs dinámico

### Enlazado estático (lo habitual)

```bash
gcc main.c juego.c -o juego
```

El **linker** toma todos los objetos y produce un único ejecutable. Cada llamada
a función queda resuelta a una dirección concreta dentro del binario, **antes de
que el programa exista**. Esa decisión es irreversible: no hay forma de cambiar
el código sin cambiar el archivo y volver a arrancar.

### Enlazado dinámico en tiempo de carga

```bash
gcc main.c -o juego -lm     # libm.so se resuelve al arrancar
```

El ejecutable guarda "necesito `sqrt` de `libm.so`". El *dynamic loader*
(`ld.so`) resuelve eso al arrancar el proceso. Ya es más tarde que el caso
anterior, pero sigue siendo antes del `main`.

### Enlazado dinámico en tiempo de ejecución

```c
void *lib = dlopen("./game.so", RTLD_NOW);
void (*f)(void) = dlsym(lib, "game_step");
```

Acá el programa decide **cuándo** cargar y **qué** cargar, en pleno
funcionamiento. Y si puede decidirlo una vez, puede decidirlo mil veces.

> **Idea clave:** hot reload no es una característica del lenguaje ni un truco
> del compilador. Es simplemente usar la tercera forma de enlazado en un bucle.

---

## 3. Qué es realmente un `.so`

Un *shared object* es un archivo ELF con código compilado que el sistema
operativo puede mapear en el espacio de direcciones de un proceso.

### Secciones relevantes

| Sección | Contenido | ¿Sobrevive a una recarga? |
|---|---|---|
| `.text` | El código de las funciones | **No** — se desmapea |
| `.rodata` | Constantes, literales de cadena | **No** |
| `.data` | Globales inicializadas | **No** |
| `.bss` | Globales sin inicializar | **No** |
| `.dynsym` | Tabla de símbolos exportados | **No** |

Todo lo que vive dentro del `.so` desaparece al descargarlo. Esto no es un
detalle: es la restricción que define toda la arquitectura.

### `-fPIC`: código independiente de la posición

```bash
gcc -shared -fPIC game.c -o game.so
```

`-fPIC` (*Position Independent Code*) genera código que funciona sin importar en
qué dirección de memoria termine mapeado. En vez de direcciones absolutas usa
desplazamientos relativos y tablas de indirección (GOT/PLT).

Es obligatorio porque el host decide la dirección de carga recién en runtime, y
además esa dirección **cambia entre recargas** (ASLR y disponibilidad de espacio).

Corolario práctico: nunca guardar una dirección obtenida con `dlsym` más allá de
la recarga siguiente. Hay que volver a pedirla siempre.

### Símbolos exportados vs internos

Solo las funciones **no `static`** aparecen en la tabla de símbolos dinámicos.

```c
// game.c
void game_step(GameState *gs, int input) { ... }   // exportada, dlsym la ve
static void move_player(GameState *gs, int d) { ... } // interna, invisible
```

Esto es deliberado en `dungeon`: la superficie pública son tres funciones
(`game_init`, `game_render`, `game_step`) y todo lo demás es `static`. Cuanto más
chica la interfaz, menos cosas pueden romperse en una recarga.

Verificación:

```bash
nm -D --defined-only game.so | rg ' T '
```

---

## 4. La API de carga dinámica

Declarada en `<dlfcn.h>`. En glibc ≥ 2.34 está incorporada a libc, por lo que
`-ldl` es opcional (inofensivo si se deja, y necesario en sistemas viejos).

### `dlopen`

```c
void *handle = dlopen("./game_live.so", RTLD_NOW);
```

Mapea la biblioteca y devuelve un handle opaco, o `NULL` si falla.

| Flag | Significado |
|---|---|
| `RTLD_NOW` | Resuelve **todos** los símbolos al cargar. Falla acá si falta algo. |
| `RTLD_LAZY` | Resuelve cada símbolo en su primera llamada. |
| `RTLD_LOCAL` | Los símbolos no quedan disponibles para otras bibliotecas (por defecto). |
| `RTLD_GLOBAL` | Los símbolos quedan visibles globalmente. |

**Para hot reload conviene `RTLD_NOW`.** Con `RTLD_LAZY` un símbolo faltante no
falla al cargar: revienta más tarde, en medio de una partida, en el momento menos
conveniente. Es preferible el fallo inmediato y verificable.

### `dlsym`

```c
void *dir = dlsym(handle, "game_step");
```

Busca un símbolo **por su nombre en texto** y devuelve su dirección.

Detalle importante: `dlsym` puede devolver `NULL` de forma **legítima** (un
símbolo cuyo valor es cero). Por eso el chequeo de errores correcto no es
comparar contra `NULL`, sino usar el protocolo de `dlerror`:

```c
dlerror();                                    // limpiar error previo
*(void **)(&code->step) = dlsym(handle, "game_step");
char *err = dlerror();                        // consultar
if (err) { /* fallo real */ }
```

`dlerror` devuelve el mensaje del último error **y lo limpia**. Llamarlo dos
veces seguidas devuelve `NULL` la segunda vez.

### El casteo raro

```c
*(void **)(&code->step) = dlsym(code->handle, "game_step");
```

Se ve horrible, y tiene una razón. El estándar ISO C **no garantiza** que un
`void *` (puntero a dato) se pueda convertir a puntero a función: son espacios de
direcciones potencialmente distintos. POSIX sí lo exige, pero `gcc -Wpedantic`
igual advierte sobre la conversión directa.

El rodeo por `void **` escribe los bytes en la variable sin pedir una conversión
que el estándar no bendice. La alternativa más limpia:

```c
union { void *obj; void (*fn)(GameState *, int); } conv;
conv.obj = dlsym(handle, "game_step");
code->step = conv.fn;
```

### `dlclose`

```c
dlclose(handle);
```

Decrementa el contador de referencias. **Si llega a cero, desmapea la
biblioteca.** Y ahí es cuando `.text`, `.data`, `.bss` y `.rodata` dejan de
existir.

Advertencia real: `dlclose` **no siempre descarga**. No lo hace si

- la biblioteca fue abierta con `RTLD_NODELETE`;
- otra biblioteca cargada depende de ella;
- usa TLS (`__thread`) o símbolos `STB_GNU_UNIQUE` (frecuente en C++).

Si `dlclose` no descarga, el código viejo sigue en memoria y la recarga
silenciosamente no hace nada. En C plano con una biblioteca sin dependencias
raras, funciona.

---

## 5. Punteros a función: el interruptor

```c
void (*step)(GameState *, int);
```

Lectura de adentro hacia afuera: `step` es un puntero a una función que toma
`GameState *` e `int` y no devuelve nada.

No es una función: es **una variable que guarda una dirección**. Al escribir
`code.step(gs, input)`, el programa lee la dirección guardada en esa variable y
salta ahí.

Por eso recargar es reasignar una variable:

```c
dlclose(code->handle);
code->handle = dlopen(LIB_LIVE, RTLD_NOW);
*(void **)(&code->step) = dlsym(code->handle, "game_step");
```

El bucle del host no cambia una línea. Sigue llamando `code.step(...)`. Cambió el
destino, no el llamador.

Ejemplo mínimo, sin juego de por medio:

```c
#include <dlfcn.h>
#include <stdio.h>

int main(void) {
    void *lib = dlopen("./saludo.so", RTLD_NOW);
    if (!lib) { fprintf(stderr, "%s\n", dlerror()); return 1; }

    void (*saludar)(void);
    *(void **)(&saludar) = dlsym(lib, "saludar");

    saludar();          // ejecuta la versión actual
    dlclose(lib);
    return 0;
}
```

```c
// saludo.c  →  gcc -shared -fPIC saludo.c -o saludo.so
#include <stdio.h>
void saludar(void) { printf("hola\n"); }
```

Cambiar el `printf`, recompilar, y el siguiente `dlopen` imprime otra cosa. Todo
el mecanismo está en esas diez líneas.

---

## 6. La separación host / lógica

La arquitectura tiene dos capas con responsabilidades disjuntas.

```
┌─────────────────────────────────────────┐
│  host  (ejecutable, nunca se recarga)   │
│                                          │
│  • dueño de la memoria del estado       │
│  • entrada/salida (input, archivos)     │
│  • ciclo dlopen / dlsym / dlclose       │
│  • detección de cambios                 │
└──────────────┬──────────────────────────┘
               │  GameState *  (puntero al estado)
               ▼
┌─────────────────────────────────────────┐
│  game.so  (se recarga cuantas veces sea)│
│                                          │
│  • reglas del juego                      │
│  • render                                │
│  • sin estado propio                     │
└─────────────────────────────────────────┘
```

Esta división no es arbitraria: es la misma que separa *plataforma* de *dominio*
en cualquier arquitectura por capas. La plataforma es lo estable y lo que habla
con el mundo exterior; el dominio es lo que cambia todo el tiempo. El hot reload
solo vuelve esa frontera obligatoria y verificable — si la lógica accede a algo
del sistema por su cuenta, la recarga lo va a delatar.

### La interfaz

```c
// game.h — el único contrato entre las dos capas
void game_init(GameState *gs);
void game_render(GameState *gs);
void game_step(GameState *gs, int input);
```

Tres símbolos. Cuanta menos superficie, menos puede romperse.

### Por qué `render` y `step` están separados

La primera versión de `dungeon` tenía una sola función `game_update(gs)` que leía
el input adentro. Consecuencia:

```
render + scanf (bloquea) ──► [se edita y compila] ──► scanf retorna
   ──► la acción se ejecuta con el CÓDIGO VIEJO
   ──► recién ahí el host chequea el mtime y recarga
```

La recarga entraba un turno tarde. Con la lectura de input en el host:

```c
while (gs->playing) {
    code.render(gs);            // dibuja
    int input = get_input();    // bloquea ← ventana para recompilar
    if (cambio()) recargar();   // chequeo DESPUÉS del input
    code.step(gs, input);       // la acción usa el código NUEVO
}
```

Lección general: **el punto de recarga tiene que estar entre la observación del
cambio y el uso del código.** Si hay una espera bloqueante, el chequeo va después
de la espera, no antes.

Como beneficio secundario, mover el input al host es lo correcto por diseño: la
entrada/salida es responsabilidad de la plataforma.

---

## 7. El estado: la regla central

> **El estado que debe sobrevivir a una recarga no puede vivir dentro del `.so`.**

La versión original de `dungeon.c` tenía:

```c
Player player;             // global en el .so
Room rooms[MAX_ROOMS];     // global en el .so
```

Esas variables viven en `.data`/`.bss` de la biblioteca. Al hacer `dlclose` se
desmapean. Cada recarga borraría la partida entera.

La solución es agrupar todo en una struct que el host reserva:

```c
// game.h
typedef struct {
    int initialized;
    int playing;
    Player player;
    Room rooms[MAX_ROOMS];
} GameState;
```

```c
// host.c
GameState *gs = calloc(1, sizeof(GameState));   // memoria del host, en el heap
```

El heap pertenece al **proceso**, no a la biblioteca. `dlclose` no lo toca. El
`.so` recibe el puntero, opera sobre esa memoria y termina sin quedarse con nada.

### Regla de trabajo

Dentro de `game.c`:

- ✅ variables locales (viven en la pila, se crean y mueren dentro de la llamada)
- ✅ `const` de solo lectura
- ✅ escribir en `gs->...`
- ❌ globales mutables
- ❌ `static` locales dentro de funciones

El punto de los `static` locales merece énfasis porque es traicionero:

```c
static void ataque_enemigo(GameState *gs) {
    static int golpes = 0;    // vive en .bss del .so
    golpes++;                 // vuelve a 0 en cada recarga
}
```

Compila, se ve inocente, y produce un bug que aparece solo después de recargar.

---

## 8. Qué sobrevive y qué no

Tabla de referencia:

| Cosa | ¿Sobrevive al `dlclose`? | Por qué |
|---|---|---|
| `malloc` hecho desde `game.c` | **Sí** | El heap es del proceso; `malloc` vive en libc |
| `GameState` del host | **Sí** | Está en el heap del host |
| Descriptores de archivo abiertos | **Sí** | Los administra el kernel, por proceso |
| Globales de `game.c` | **No** | `.data`/`.bss` del `.so`, se desmapean |
| `static` locales de `game.c` | **No** | Ídem |
| Literales de cadena de `game.c` | **No** | `.rodata` del `.so` |
| Punteros a funciones de `game.c` | **No** (quedan colgando) | `.text` del `.so` |

Las dos últimas filas son la fuente de los bugs más difíciles de esta técnica.

### Punteros colgantes a `.rodata`

```c
// PELIGROSO
gs->rooms[0].name_ptr = "Entrada";   // apunta a .rodata del .so
```

Tras la recarga, esa dirección apunta a memoria desmapeada. Leerla es
*segmentation fault* si hay suerte, o basura silenciosa si no la hay.

Por eso `dungeon` usa arreglos de tamaño fijo y **copia** el contenido:

```c
char name[50];
...
snprintf(r->name, sizeof(r->name), "%s", name);   // copia los bytes al estado
```

Los bytes quedan dentro del `GameState`, en el heap del host. Sobreviven.

### Punteros colgantes a funciones

Una tabla de comandos guardada en el estado es exactamente el mismo problema:

```c
// PELIGROSO
typedef struct {
    char tecla;
    void (*accion)(GameState *);   // apunta a .text del .so
} Comando;

typedef struct {
    Comando comandos[10];          // guardado en GameState → colgará
} GameState;
```

Si la tabla tiene que existir, se reconstruye al principio de cada `game_step`, o
se guardan índices/enums en vez de punteros y se resuelven con un `switch`.

### Sobre `malloc` dentro del `.so`

La memoria reservada desde `game.c` **no se pierde**: `malloc` no pertenece a
`game.so` sino a libc, y el heap es del proceso. El riesgo no es que se invalide,
sino organizativo: si `game.c` reserva memoria y el puntero se guarda en el
`GameState`, hay que asegurar que nadie asuma que el bloque tiene un layout que
cambió entre versiones, y que quede claro quién libera. Lo más simple, y lo que
hace `dungeon`, es que el estado sea de tamaño fijo y no haya asignaciones
dinámicas en la capa de lógica.

---

## 9. Layout de structs y compatibilidad binaria

El `GameState` sobrevive como **bytes crudos**. La struct es solo el molde para
interpretarlos. Si el molde cambia y los bytes no, se lee mal.

```c
// Versión A (los bytes en memoria se escribieron con este molde)
typedef struct {
    char name[50];
    int hp;         // offset 50 (más padding)
    int max_hp;
} Player;

// Versión B (se recarga con este molde)
typedef struct {
    char name[50];
    int nivel;      // ← insertado en el medio
    int hp;         // ahora en el offset donde antes estaba max_hp
    int max_hp;     // lee basura
} Player;
```

Nada crashea. Simplemente los valores dejan de tener sentido, y el bug parece de
lógica cuando en realidad es de memoria. Es de los peores ratos que da esta
técnica.

### Reglas de modificación

| Cambio | ¿Seguro en caliente? |
|---|---|
| Modificar el cuerpo de una función | ✅ Sí, es el caso normal |
| Agregar una función nueva | ✅ Sí |
| Agregar un campo **al final** de `GameState` | ⚠️ Suele funcionar (el `calloc` dejó ceros solo si el campo entra en el tamaño ya reservado) |
| Agregar un campo en el medio de cualquier struct | ❌ Reiniciar el host |
| Cambiar el tipo o el tamaño de un campo | ❌ Reiniciar el host |
| Reordenar campos | ❌ Reiniciar el host |
| Cambiar la firma de `game_init`/`render`/`step` | ❌ Reiniciar el host (y recompilar) |

Notar que "agregar al final" es seguro solo si el bloque reservado ya tiene ese
espacio. Como el host hizo `calloc(1, sizeof(GameState))` con el tamaño **viejo**,
un campo nuevo cae fuera del bloque. Lo prudente es reservar de más desde el
principio:

```c
typedef struct {
    /* ... campos reales ... */
    char reservado[4096];   // colchón para agregar campos sin reiniciar
} GameState;
```

### Detección automática de incompatibilidad

Un patrón útil: versionar el estado y que el host lo verifique.

```c
// game.h
#define GAME_STATE_VERSION 3

// game.c
int game_state_version(void) { return GAME_STATE_VERSION; }
size_t game_state_size(void) { return sizeof(GameState); }
```

```c
// host.c, después de cada dlsym
if (code.state_version() != esperado || code.state_size() != sizeof(GameState)) {
    fprintf(stderr, "[host] layout incompatible, reiniciá el host\n");
}
```

Convierte un bug silencioso de memoria en un mensaje de error legible. Vale la
pena en cuanto el `GameState` empieza a crecer.

---

## 10. Detección de cambios

### Polling de `mtime`

Es lo que usa `dungeon`, y para un juego por turnos alcanza:

```c
static time_t lib_mtime(void) {
    struct stat st;
    if (stat(LIB_PATH, &st) != 0) return 0;
    return st.st_mtime;
}
```

Se guarda el `mtime` del momento de la carga y se compara en cada iteración. Si
cambió, hay código nuevo.

Ventajas: portable, trivial, sin dependencias.
Desventaja: es un sondeo, y `st_mtime` tiene resolución de un segundo (se puede
usar `st.st_mtim.tv_nsec` para más precisión).

### `inotify` (Linux)

Para no sondear, el kernel puede avisar:

```c
int fd = inotify_init1(IN_NONBLOCK);
inotify_add_watch(fd, ".", IN_CLOSE_WRITE);
// leer eventos de fd sin bloquear en cada frame
```

`IN_CLOSE_WRITE` es especialmente apropiado: se dispara cuando alguien que
escribió el archivo **lo cierra**, es decir cuando terminó de escribir. Resuelve
de paso el problema de la sección siguiente.

Más complejo, y solo vale la pena en un bucle de 60 fps donde un `stat` por frame
empieza a notarse.

---

## 11. El problema del archivo en uso

Mientras una biblioteca está mapeada, ese archivo está siendo usado por el
proceso. Si el compilador lo reescribe justo en ese momento, se están pisando
páginas que alguien puede estar ejecutando.

Además hay una carrera más sutil: el `mtime` cambia en cuanto el compilador
**empieza** a escribir. Si se hace `dlopen` en ese instante, se carga un ELF
truncado.

### Solución: cargar una copia

```c
static int copy_lib(void) {
    FILE *src = fopen(LIB_PATH, "rb");    // game.so
    FILE *dst = fopen(LIB_LIVE, "wb");    // game_live.so
    /* copia por bloques */
}
```

El host carga **`game_live.so`**, nunca `game.so`. Así `make` puede reescribir el
original con total libertad. El `mtime` del original queda como pura señal.

### Solución complementaria: reintentos

```c
static int load_game_retry(GameCode *code) {
    struct timespec pausa = {0, 100 * 1000 * 1000};   // 100 ms
    for (int i = 0; i < RELOAD_RETRIES; i++) {
        if (load_game(code)) return 1;
        nanosleep(&pausa, NULL);
    }
    return 0;
}
```

Si se agarra el archivo a medio escribir, `dlopen` falla, se espera y se
reintenta. Cinco intentos con 100 ms cubren cualquier compilación normal.

### Variante: nombres únicos

Otra estrategia, común en motores más grandes, es copiar a un nombre distinto
cada vez:

```c
snprintf(ruta, sizeof(ruta), "./game_live_%d.so", generacion++);
```

Evita por completo el riesgo de escribir sobre un archivo mapeado, al precio de
tener que limpiar los archivos viejos. Con la copia única alcanza mientras se
descargue antes de copiar, que es el orden que usa `load_game`.

---

## 12. El punto de recarga seguro

No se puede recargar en cualquier momento. El requisito es que **ninguna función
del `.so` esté en la pila de llamadas** cuando se hace `dlclose`. Si se descarga
la biblioteca mientras una de sus funciones está a medio ejecutar, el retorno
salta a memoria desmapeada.

En `dungeon` esto está garantizado por construcción:

```c
while (gs->playing) {
    code.render(gs);            // ← retorna (nada del .so en la pila)
    int input = get_input();    // ← código del host
    if (cambio()) recargar();   // ← seguro
    code.step(gs, input);       // ← retorna
}
```

La recarga ocurre en el nivel superior del bucle, con la pila limpia de código
del juego.

Un juego por turnos con entrada bloqueante es el caso más cómodo: el proceso
queda detenido en un punto conocido, sin nada a medio hacer, esperando. Es la
ventana ideal.

En un bucle de tiempo real (60 fps) el principio no cambia — el chequeo va entre
frames, nunca dentro del procesamiento de uno.

---

## 13. Catálogo de errores

### `dlopen` devuelve `NULL`

```c
if (!handle) fprintf(stderr, "%s\n", dlerror());
```

Causas frecuentes:

- **Ruta sin `./`** — `dlopen("game.so")` busca en las rutas del sistema, no en
  el directorio actual. Hay que escribir `dlopen("./game.so")`.
- Archivo a medio escribir (ver sección 11).
- Falta `-fPIC` al compilar la biblioteca.
- Símbolo sin resolver, con `RTLD_NOW`.

### `dlsym` devuelve `NULL`

- La función es `static` → no se exporta. Quitar el `static`.
- El nombre está mal escrito (es un string, el compilador no lo verifica).
- El código está compilado como C++ sin `extern "C"` → *name mangling*.

### El estado se resetea en cada recarga

Hay estado viviendo dentro del `.so`: una global, o un `static` local. Buscar con:

```bash
rg '^\s*(static\s+)?\w+\s+\w+\s*(\[|=|;)' game.c   # candidatos a global
```

### Valores absurdos después de recargar

Cambió el layout de una struct compartida. Reiniciar el host. Si pasa seguido,
implementar la verificación de versión de la sección 9.

### Segfault al leer un nombre o descripción

Puntero a `.rodata` del `.so` guardado en el estado. Copiar los bytes en vez de
guardar el puntero.

### La recarga "no hace nada"

- `dlclose` no descargó realmente (ver sección 4).
- Se está recompilando pero el host mira otro archivo.
- El `mtime` no cambió porque la compilación falló. Conviene mirar la salida de
  `make`, no asumir que anduvo.

### El código nuevo se aplica un turno tarde

El chequeo del `mtime` está antes de la espera bloqueante en vez de después. Ver
sección 6.

---

## 14. Herramientas de diagnóstico

```bash
# Símbolos exportados por la biblioteca
nm -D --defined-only game.so

# Dependencias y direcciones de carga
ldd game.so

# Confirmar que es una biblioteca compartida válida
file game.so

# Ver los .so mapeados por el proceso vivo (en otra terminal)
rg 'game' /proc/$(pgrep -f '^./host')/maps

# Traza de resolución dinámica
LD_DEBUG=libs,symbols ./host 2>&1 | head -50
```

`/proc/<pid>/maps` es especialmente útil: muestra en vivo si la copia vieja se
desmapeó de verdad y en qué dirección quedó la nueva.

---

## 15. Limitaciones y alternativas

### Lo que el hot reload no resuelve

- Cambios en el layout del estado.
- Cambios en la interfaz entre host y biblioteca.
- Bugs en el host (hay que reiniciar igual).
- Depuración cómoda: GDB se confunde con los símbolos al recargar.

### Alternativa: serializar y reiniciar

```c
guardar_estado("save.bin", gs);   // fwrite del GameState
// recompilar todo, reiniciar
cargar_estado("save.bin", gs);    // fread
```

Se pierde el "en vivo" real, pero:

- es muchísimo más simple (no hay `dlopen`, ni `.so`, ni separación forzada);
- **soporta cambios de layout**, si el formato se versiona;
- funciona igual para bugs en el host.

Para un juego por turnos de consola es una opción perfectamente competitiva. El
hot reload gana cuando el estado es caro de reconstruir o cuando se está
ajustando algo que requiere ver el efecto inmediato (balance, timing, feel).

### Alternativa: lenguaje embebido

Escribir la lógica de juego en Lua y mantener el motor en C. El intérprete recarga
scripts sin nada de esto. Es lo que hacen muchos motores comerciales. Costo:
sumar una dependencia y el puente entre los dos lenguajes.

---

## 16. Checklist

Antes de recargar:

- [ ] El estado vive en el `GameState` del host, no en globales del `.so`
- [ ] Sin `static` locales mutables en `game.c`
- [ ] Sin punteros a `.rodata` o a funciones del `.so` guardados en el estado
- [ ] Solo cambiaron cuerpos de funciones, no layouts de structs
- [ ] La compilación terminó bien (`make` sin errores)
- [ ] La recarga ocurre con la pila limpia de código del `.so`

Al armar el proyecto:

- [ ] `-shared -fPIC` para la biblioteca
- [ ] `RTLD_NOW` en `dlopen`
- [ ] Ruta con `./` explícito
- [ ] Protocolo `dlerror()` antes y después de `dlsym`
- [ ] Cargar una copia, no el archivo que el compilador reescribe
- [ ] Reintentos ante fallo de carga
- [ ] Interfaz mínima entre las dos capas

---

## 17. Recorrido por el código de `dungeon`

Esta sección reconstruye la arquitectura en el orden en que se piensa, no en el
orden en que quedan los archivos. La idea es que se vea de dónde sale cada
decisión.

### 17.1 El criterio de partición

La pregunta que ordena todo es una sola:

> **¿Esto tiene que sobrevivir a la recarga?**

Si la respuesta es *sí*, va al host. Si es *no*, va a la biblioteca. Aplicándola
a las piezas del juego:

| Pieza | ¿Sobrevive? | Destino |
|---|---|---|
| El bucle principal | Sí — es lo que nunca se detiene | `host.c` |
| La memoria de la partida | Sí — es el punto entero del ejercicio | `host.c` (reserva) |
| La lectura del teclado | Sí — es I/O de plataforma | `host.c` |
| El ciclo `dlopen`/`dlsym` | Sí — no puede recargarse a sí mismo | `host.c` |
| Las reglas del combate | No — es lo que se quiere editar | `game.c` |
| El dibujo de la habitación | No | `game.c` |
| La forma de las structs | Ninguno de los dos: **la comparten** | `game.h` |

Esa última fila es la que obliga a que existan **tres** archivos y no dos. El
host necesita conocer `sizeof(GameState)` para reservarlo, y la lógica necesita
conocer los campos para usarlos. Un header compartido es la única forma de que
ambos vean exactamente el mismo molde.

Y de ahí sale una consecuencia que conviene tener presente: `game.h` es el punto
de acoplamiento. Es el archivo cuyo cambio obliga a recompilar **las dos** capas
—y por eso el `Makefile` lo lista como dependencia de ambos objetivos.

### 17.2 `game.h` — el contrato

Se arma en tres bloques, de lo más estable a lo más específico.

**Bloque 1: constantes compartidas.** Colores ANSI y tamaños. Van acá porque las
usan las dos capas (el host imprime sus mensajes en verde y rojo).

```c
#define GREEN "\033[32m"
#define MAX_ITEMS 50
#define MAX_ROOMS 8
```

**Bloque 2: las structs de datos.** Son las que ya estaban en el `dungeon.c`
original, movidas tal cual salvo la corrección de `enemigo_nombre`:

```c
typedef struct {
    char name[50];
    char descripcion[MAX_DESC];
    int norte, sur, este, oeste;   // -1 = no hay salida
    Items items[MAX_ITEMS];
    int item_cont;
    int enemigo_hp;
    char enemigo_nombre[50];       // era `int`, bug del original
    int enemigo_ataque;
    int limpieza;                  // 1 = sin enemigo vivo
} Room;
```

Detalle de diseño que ya no es casual: **todo es de tamaño fijo**. `char
name[50]` y no `char *name`. Eso ocupa más memoria y es menos elegante, pero
significa que los bytes viven *dentro* del estado. Si fueran punteros a literales
del `.so`, quedarían colgando en la primera recarga (sección 8).

**Bloque 3: el estado y la interfaz.**

```c
typedef struct {
    int initialized;
    int playing;
    Player player;
    Room rooms[MAX_ROOMS];
} GameState;

void game_init(GameState *gs);
void game_render(GameState *gs);
void game_step(GameState *gs, int input);
```

Los dos `int` de arriba son de control, no de juego:

- `initialized` — para que `game_init` corra una sola vez en toda la ejecución,
  sin importar cuántas recargas haya en el medio.
- `playing` — el canal por el que la lógica le avisa al host que corte el bucle.
  El `.so` no puede llamar a `exit()` por su cuenta: el host tiene que liberar la
  memoria y descargar la biblioteca ordenadamente. Así que "quiero terminar" es
  un dato en el estado, no una acción.

### 17.3 `host.c` — construido de abajo hacia arriba

El host se arma en cinco capas, cada una apoyada en la anterior.

**Capa 1 — el paquete de código cargado.** Antes que nada hay que decidir qué
significa "tener el juego cargado":

```c
typedef struct {
    void *handle;                      // lo que devolvió dlopen
    void (*init)(GameState *);
    void (*render)(GameState *);
    void (*step)(GameState *, int);
    time_t stamp;                      // mtime de game.so al momento de cargar
} GameCode;
```

Agrupar los punteros en una struct no es cosmético: garantiza que el handle y los
tres punteros se actualicen **juntos**. Un puntero de la versión vieja conviviendo
con dos de la nueva es un crash difícil de diagnosticar.

El `stamp` va acá y no en una variable aparte por la misma razón: es parte de la
identidad de lo que está cargado.

**Capa 2 — I/O de plataforma.** El input, que se mudó desde `game.c`:

```c
static int get_input(void) {
    int valor;
    if (scanf("%d", &valor) != 1) {
        if (feof(stdin)) return 0;     // sin stdin, salir limpio
        clear_input_buffer();
        return -1;                     // basura, el juego lo rechaza
    }
    clear_input_buffer();
    return valor;
}
```

El `clear_input_buffer` existe porque `scanf("%d")` deja el `\n` en el buffer; sin
limpiarlo, la siguiente lectura lo consume y el juego "salta" un turno. Es un
clásico de C que no tiene nada que ver con el hot reload, pero rompe igual.

El caso `feof` es lo que permite que el juego se pueda probar con una tubería
(`printf '5\n0\n' | ./host`) sin quedar en un bucle infinito cuando se acaba la
entrada.

**Capa 3 — mecánica de archivos.**

```c
static time_t lib_mtime(void);   // ¿cuándo se modificó game.so?
static int copy_lib(void);       // game.so → game_live.so
```

Dos funciones chicas y sin estado. `lib_mtime` devuelve `0` si el archivo no
existe, lo que se compara distinto de cualquier `stamp` válido y por lo tanto
dispara un intento de recarga: falla ruidosamente en vez de ignorar el problema.

**Capa 4 — el ciclo de vida de la biblioteca.** Acá se usa todo lo anterior:

```c
static void unload_game(GameCode *code) {
    if (code->handle) dlclose(code->handle);
    code->handle = NULL;
    code->init = NULL;
    code->render = NULL;
    code->step = NULL;      // dejar los punteros en NULL, no colgando
}
```

Poner los punteros en `NULL` después de descargar es disciplina barata: si algún
camino del código intenta usarlos, el crash es inmediato y obvio en vez de saltar
a memoria desmapeada con síntomas aleatorios.

```c
static int load_game(GameCode *code) {
    unload_game(code);                     // 1. sacar el viejo PRIMERO
    if (!copy_lib()) return 0;             // 2. copiar (ya nadie lo tiene mapeado)
    code->handle = dlopen(LIB_LIVE, RTLD_NOW);   // 3. cargar la copia
    if (!code->handle) { ... }
    dlerror();                             // 4. limpiar antes de dlsym
    *(void **)(&code->init)   = dlsym(code->handle, "game_init");
    *(void **)(&code->render) = dlsym(code->handle, "game_render");
    *(void **)(&code->step)   = dlsym(code->handle, "game_step");
    char *err = dlerror();                 // 5. un solo chequeo para los tres
    if (err) { unload_game(code); return 0; }
    code->stamp = lib_mtime();             // 6. recién ahora, marcar la versión
    return 1;
}
```

El orden importa en dos puntos concretos:

- **Descargar antes de copiar.** Si se copiara primero, se estaría escribiendo
  sobre el archivo que el proceso todavía tiene mapeado.
- **`stamp` al final.** Si se guardara al principio y la carga fallara, el host
  creería tener cargada una versión que en realidad nunca cargó, y no volvería a
  intentarlo.

Y encima de eso, la tolerancia a fallos:

```c
static int load_game_retry(GameCode *code) {
    for (int i = 0; i < RELOAD_RETRIES; i++) {
        if (load_game(code)) return 1;
        nanosleep(&pausa, NULL);           // 100 ms
    }
    return 0;
}
```

Está separada de `load_game` a propósito: una función decide *cómo cargar*, la
otra decide *cuánto insistir*. Son dos preocupaciones distintas y conviene poder
cambiar una sin tocar la otra.

**Capa 5 — el `main`.** Con todo lo anterior, queda casi declarativo:

```c
int main(void) {
    srand(time(NULL));                     // aleatoriedad: del host, no del .so

    GameCode code = {0};
    if (!load_game_retry(&code)) return 1;

    GameState *gs = calloc(1, sizeof(GameState));   // ← la memoria que sobrevive
    gs->playing = 1;

    if (!gs->initialized) {
        code.init(gs);
        gs->initialized = 1;
    }

    while (gs->playing) { /* ver 17.6 */ }

    free(gs);
    unload_game(&code);
}
```

Tres decisiones para señalar:

1. **`srand` está en el host.** Si estuviera en `game_init`, cada recarga podría
   resembrar el generador y la aleatoriedad dejaría de ser reproducible. La
   semilla es estado de proceso.
2. **`calloc` y no `malloc`.** `calloc` pone todo en cero, lo que hace que
   `initialized` valga `0` sin necesidad de asignarlo, y que todos los campos del
   juego arranquen en un valor definido. Con `malloc` se estaría leyendo basura.
3. **El chequeo de `initialized` parece redundante** —después de un `calloc`
   siempre es cero— y lo es hoy. Está puesto porque el día que el estado se cargue
   de un archivo de guardado, `game_init` no debe volver a correr, y el `main` ya
   está escrito para eso.

### 17.4 `game.c` — el orden interno

La lógica se organiza en cuatro niveles, del más genérico al más específico.

**Nivel 1: prototipos internos, todos `static`.**

```c
static void init_rooms(GameState *gs);
static void display_room(GameState *gs);
static void move_player(GameState *gs, int direction);
/* ... */
```

Son los mismos prototipos del `dungeon.c` original, con dos cambios sistemáticos:

- `static`, para que **no** se exporten. Solo tres símbolos salen de la
  biblioteca; el resto es asunto interno.
- Todos reciben `GameState *gs`. Es la traducción mecánica de "ya no hay
  globales": lo que antes era una variable de archivo, ahora es un parámetro.

Ese segundo cambio es la parte más tediosa de convertir un programa existente a
esta arquitectura, y también la más mecánica. Cada `player.hp` pasa a ser
`gs->player.hp`.

**Nivel 2: constructores.** Funciones que arman datos sin tocar el estado global:

```c
static Items make_item(const char *nombre, const char *desc,
                       int heal, int dmg, int value) {
    Items it = {0};
    snprintf(it.nombre, sizeof(it.nombre), "%s", nombre);
    /* ... */
    return it;
}
```

`snprintf` en vez de `strcpy` por dos razones: no puede desbordar el arreglo, y
—clave para este ejercicio— **copia los bytes** en vez de guardar el puntero al
literal. El literal `"Antorcha"` vive en `.rodata` del `.so` y desaparece en la
recarga; la copia dentro del `Items` vive en el `GameState` y sobrevive.

Devolver la struct por valor (y no un puntero) es la misma idea aplicada: no hay
nada que pueda quedar colgando.

**Nivel 3: la API pública.** Las tres funciones que el host resuelve con `dlsym`.

`game_init` arma la partida inicial:

```c
void game_init(GameState *gs) {
    Player *p = &gs->player;
    p->max_hp = 30;  p->hp = p->max_hp;
    p->attack = 5;   p->defense = 2;
    /* ... */
    init_rooms(gs);
    display_welcome();
}
```

`game_render` solo imprime; no modifica nada. Esa separación es la que permite
llamarlo antes de leer el input sin que avance la partida.

`game_step` es el `switch` del `main` original, ahora recibiendo la opción ya
leída:

```c
void game_step(GameState *gs, int choice) {
    switch (choice) {
    case 1: move_player(gs, 0); break;
    /* ... */
    case 0: gs->playing = 0; return;      // salir: return, no exit()
    default: printf(RED "Opcion invalida.\n" RESET); break;
    }
    chequeo_muerte(gs);
    chequeo_victorias(gs);
}
```

Dos detalles:

- El `case 0` hace `return` temprano para saltear los chequeos de fin de partida.
  Si se sale voluntariamente no tiene sentido evaluar victoria o derrota.
- Los dos `chequeo_*` van **después** del `switch`, no dentro de cada caso. Son
  reglas que valen para cualquier acción: cualquier cosa que se haga puede
  terminar la partida.

**Nivel 4: las implementaciones.** El resto. Nada especial salvo la disciplina de
no declarar globales ni `static` locales mutables.

### 17.5 `Makefile`

```makefile
all: host game.so

host: host.c game.h
	$(CC) $(CFLAGS) -o host host.c -ldl

game.so: game.c game.h
	$(CC) $(CFLAGS) -shared -fPIC -o game.so game.c
```

Lo importante son las **dependencias declaradas**, que no son decorativas:

- `host` depende de `game.h` porque necesita `sizeof(GameState)`. Si cambia el
  header, el host quedó con el molde viejo y hay que recompilarlo.
- `game.so` depende de `game.h` por lo mismo, del otro lado.

Ese `game.h` compartido en las dos reglas es exactamente el mecanismo que evita
el bug de la sección 9: si se cambia el layout y se recompilan las dos capas, no
hay incompatibilidad. El problema aparece solo cuando se recompila **una sola**,
que es justamente lo que pasa cuando se corre `make game.so` en caliente.

Por eso la regla práctica: `make game.so` sirve para cambios en la lógica.
Cambios en `game.h` piden `make` completo y reiniciar el host.

Notar también que hay dos objetivos separados a propósito. `make game.so`
recompila solo la biblioteca, que es lo que se quiere durante una sesión de
juego; `make` a secas reconstruye todo.

### 17.6 El flujo completo, seguido paso a paso

```
./host arranca
  │
  ├─ load_game_retry()
  │    ├─ copy_lib()      game.so → game_live.so
  │    ├─ dlopen(game_live.so, RTLD_NOW)
  │    ├─ dlsym × 3       init, render, step
  │    └─ stamp = mtime(game.so)
  │
  ├─ gs = calloc(1, sizeof(GameState))     ← memoria que va a sobrevivir a todo
  ├─ gs->playing = 1
  ├─ code.init(gs)                          ← una única vez
  │
  └─ while (gs->playing)
       │
       ├─ code.render(gs)          el .so dibuja habitación, stats y menú
       │
       ├─ get_input()              EL HOST ESPERA  ◄── ventana de edición
       │                             │
       │                             └─ en otra terminal: editar game.c
       │                                                  make game.so
       │
       ├─ ¿mtime(game.so) != stamp?
       │    └─ sí → load_game_retry()
       │              ├─ dlclose      el código viejo se desmapea
       │              ├─ copy_lib     se copia la versión nueva
       │              ├─ dlopen       se mapea
       │              └─ dlsym × 3    los punteros apuntan al código nuevo
       │                              (gs NO se tocó en ningún momento)
       │
       └─ code.step(gs, input)     el CÓDIGO NUEVO resuelve la acción vieja
```

La línea que resume todo: en ese bloque de recarga, `gs` no aparece nunca. El
estado no participa del proceso. Por eso sobrevive.

### 17.7 Cómo evolucionó el diseño

Vale la pena dejar registrado que la primera versión de esta arquitectura estaba
mal, porque el error es instructivo.

**Versión 1** — una sola función en la interfaz:

```c
void game_update(GameState *gs);   // dibujaba, leía el input y actuaba
```

Compilaba, cargaba, recargaba. Y sin embargo, al probarla, el código nuevo
entraba **un turno tarde**. La traza explica por qué:

```
chequeo de mtime  →  game_update()  →  render  →  scanf BLOQUEA
                                                     │
                          [acá se edita y se compila]│
                                                     ▼
                                          la acción corre con CÓDIGO VIEJO
                                                     │
                          el bucle vuelve al chequeo ┘  ← demasiado tarde
```

El chequeo estaba antes de la espera. Pero el cambio ocurre **durante** la espera.

**Versión 2** — partir la interfaz donde estaba la espera:

```c
void game_render(GameState *gs);
void game_step(GameState *gs, int input);
```

y mover `get_input` al host, que es lo que permite meter el chequeo entre las dos
llamadas.

La lección general, más allá de este caso: **el punto de recarga tiene que estar
entre el momento en que el cambio se vuelve observable y el momento en que el
código se usa.** Si hay una operación bloqueante en el medio, la interfaz tiene
que cortarse ahí. La forma de la API terminó determinada por dónde el programa se
detiene.

Y el desvío feliz: separar render de step y sacar el I/O de la lógica es lo que
uno haría igual por buen diseño. El requerimiento técnico y el buen diseño
apuntaron al mismo lugar, lo cual suele ser buena señal de que la arquitectura
está bien orientada.

---

## 18. Referencias

- `man 3 dlopen`, `man 3 dlsym`, `man 3 dlerror`
- `man 7 inotify`
- **Handmade Hero** (Casey Muratori), días 21–22 — el origen de este patrón en
  desarrollo de juegos.
- *Linkers and Loaders*, John R. Levine — cómo funciona el enlazado por dentro.
- `man ld.so` — variables de entorno del cargador dinámico (`LD_DEBUG` incluido).

---

## Apéndice: el bucle completo

```c
// host.c — la versión mínima de todo lo anterior
while (gs->playing) {
    code.render(gs);                        // el .so dibuja

    int input = get_input();                // el host espera (ventana de edición)

    if (lib_mtime() != code.stamp) {        // ¿cambió el archivo?
        unload_game(&code);                 //   sacar el viejo de memoria
        copy_lib();                         //   copiar game.so → game_live.so
        code.handle = dlopen(LIB_LIVE, RTLD_NOW);
        /* dlsym de los tres símbolos */
        code.stamp = lib_mtime();
    }

    code.step(gs, input);                   // el .so actúa, con el código nuevo
}
```

Todo lo demás de este documento es la disciplina necesaria para que estas quince
líneas no se conviertan en una fuente de bugs imposibles de encontrar.
