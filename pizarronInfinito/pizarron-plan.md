# Pizarrón Infinito de Terminales — Plan de aprendizaje y construcción (C + Rust)
https://www.linusakesson.net/programming/tty/
## Contexto

Brian quiere construir un "pizarrón infinito" estilo Obsidian Canvas, pero donde los nodos
son **terminales reales** (shells corriendo), todas visibles a la vez, con pan/zoom infinito.
Lo quiere hacer en **C + Rust** como proyecto de aprendizaje profundo: entender cada capa,
saber POR QUÉ cada decisión, y tener lecturas para estudiar antes de cada fase.
No es "que funcione rápido" — es "que funcione Y que yo entienda cada línea".

## La arquitectura y el PORQUÉ de cada decisión

El proyecto se parte en 4 capas, de abajo hacia arriba:

```
+--------------------------------------------------+
| 4. GUI: canvas infinito (Rust + egui::Scene)     |
+--------------------------------------------------+
| 3. Emulación de terminal: bytes ANSI -> grilla   |
|    de celdas (Rust, crate `vte`)                 |
+--------------------------------------------------+
| 2. Puente FFI: wrapper seguro en Rust sobre la   |
|    lib C (unsafe extern + tipo seguro con Drop)  |
+--------------------------------------------------+
| 1. libpty: pseudo-terminales POSIX (C puro)      |
|    openpty/forkpty, ioctl, termios               |
+--------------------------------------------------+
```

### ¿Por qué C en la capa 1 y no todo Rust?

**Honestidad técnica primero**: se PUEDE hacer todo en Rust (crates `nix`/`rustix` envuelven
`openpty`). La razón para hacer la capa PTY en C es **pedagógica y es válida**: la API de
pseudo-terminales ES una API de C (syscalls POSIX: `openpty(3)`, `forkpty(3)`, `ioctl(2)`,
`termios(3)`). Escribirla en C te obliga a entender file descriptors, fork/exec, señales
(SIGCHLD, SIGWINCH) sin capas de abstracción encima. Además es el excusa perfecta para
aprender FFI: unir C y Rust es una habilidad de sistemas real (así funciona medio ecosistema:
openssl, sqlite, zlib...).

**El scope de C queda chico y bien delimitado** (~150-300 líneas): eso es clave. C para lo
que C hace bien (hablar con el kernel), Rust para todo lo que tiene estado complejo (parser,
grilla, GUI) donde el borrow checker te salva.

### ¿Por qué egui y no GTK/Qt/iced?

- `egui` es immediate-mode: redibujás todo cada frame. Para un canvas con N terminales
  actualizándose constantemente, este modelo es MÁS simple que retained-mode (no hay árbol
  de widgets que sincronizar con el estado).
- `egui::Scene` (desde egui 0.31) es un contenedor pan/zoom oficial: el canvas infinito
  ya existe como primitiva. No hay que inventarlo.
- Existe `egui_term` (widget de terminal para egui sobre alacritty_terminal) como
  **implementación de referencia para LEER**, no para depender: está en 0.1.0 y en desarrollo.
  Leer su código cuando te trabás vale oro.
- `iced` es elegante pero retained/Elm-style, curva más empinada y menos material de canvas.

### ¿Por qué `vte` y no `alacritty_terminal`?

- `alacritty_terminal` te da la máquina de estados COMPLETA (grilla incluida) — rápido pero
  aprendés poco.
- `vte` (el parser que usa Alacritty por dentro) te da SOLO el parser de escape sequences:
  vos implementás el trait `Perform` y construís tu propia grilla de celdas. Ahí es donde
  entendés de verdad cómo funciona una terminal.
- **Escape hatch**: si la fase 3 se hace cuesta arriba, cambiar `vte` por
  `alacritty_terminal` es un swap localizado — la arquitectura no cambia.

### Modelo de concurrencia

Un thread lector por terminal (lee del fd master del PTY, bloqueante), manda bytes por un
`mpsc::channel` al estado compartido, y llama `ctx.request_repaint()` para despertar a egui.
La UI corre en el thread principal. Simple, sin async: tokio acá sería complejidad gratis.

---

## Fases (cada una termina en algo que FUNCIONA y se puede probar)

### Fase 0 — Conceptos (sin escribir código del proyecto)

**Objetivo**: entender qué es un PTY antes de tocar nada.

Lecturas obligatorias:
- **"The TTY demystified"** — Linus Åkesson (linusakesson.net/programming/tty) — EL artículo.
  Leelo dos veces.
- man pages: `pty(7)`, `openpty(3)`, `forkpty(3)`, `termios(3)`, `ioctl_tty(2)`.
- Si Rust es nuevo: The Rust Book caps. 1-10 (ownership, structs, enums, traits) — rust-lang.org/learn.

Experimento sin código: correr `tty` en la shell, ver `/dev/pts/N`, hacer
`echo hola > /dev/pts/N` desde otra terminal. Cuando ESO te haga clic, seguís.

### Fase 1 — `libpty` en C (~1 semana)

**Objetivo**: programa C standalone que spawnea `bash` en un PTY y hace de "cable":
lo que tipeás va al shell, lo que el shell escupe sale por stdout.

Estructura del repo:
```
pizarron/
├── libpty/
│   ├── pty.h        # API pública: pty_spawn, pty_read, pty_write, pty_resize, pty_kill
│   ├── pty.c
│   └── main_test.c  # el "cable" de prueba
└── (después) Cargo workspace
```

API a diseñar (esto ES el ejercicio de diseño):
```c
typedef struct { int master_fd; pid_t child_pid; } pty_handle;
int  pty_spawn(pty_handle *h, const char *shell, unsigned short cols, unsigned short rows);
ssize_t pty_read(pty_handle *h, char *buf, size_t len);
ssize_t pty_write(pty_handle *h, const char *buf, size_t len);
int  pty_resize(pty_handle *h, unsigned short cols, unsigned short rows);
void pty_kill(pty_handle *h);
```

Conceptos que vas a pelear acá: `forkpty()` vs `openpty()`+`fork()`+`login_tty()`,
raw mode con `termios` (para el main_test), `ioctl(fd, TIOCSWINSZ, ...)` para resize,
`waitpid` + SIGCHLD para detectar que el shell murió.

Lecturas: "Advanced Programming in the UNIX Environment" (Stevens) cap. 19 (Pseudo
Terminals) si conseguís el libro; si no, buscar "openpty forkpty tutorial" + los man pages.

**Verificación**: `./main_test` te da un bash usable donde `ls`, `vim` y `htop` funcionan.
Si htop se dibuja bien, tu PTY está bien hecho.

### Fase 2 — Puente FFI: Rust llama a tu C (~3-5 días)

**Objetivo**: crate Rust `pty-sys` + wrapper seguro `pty`.

- `pty-sys`: compila `libpty` con el crate `cc` en `build.rs`, declara los
  `unsafe extern "C" fn` a mano (a mano primero — bindgen después, para que entiendas
  qué genera).
- `pty`: tipo `Pty` seguro — `Pty::spawn()`, `read()`, `write()`, `resize()`, y `Drop`
  que llama `pty_kill` (RAII: el shell muere cuando el valor sale de scope — comparalo
  con lo que tenías que hacer a mano en C).

Lecturas: **The Rustonomicon, capítulo FFI** (doc.rust-lang.org/nomicon/ffi.html),
docs del crate `cc`, y "The Rust FFI Omnibus" (jakegoulding.com/rust-ffi-omnibus).

**Verificación**: test de integración Rust que spawnea `sh`, escribe `echo hola\n`,
lee y asserta que la salida contiene `hola`.

### Fase 3 — Emulación: bytes → grilla de celdas (~1-2 semanas, la más densa)

**Objetivo**: crate `term-model`. Struct `Grid { cells: Vec<Cell>, cols, rows, cursor }`
donde `Cell { ch, fg, bg, flags }`. Implementás el trait `Perform` de `vte`:
- `print(c)` → escribir en la celda del cursor y avanzar
- `execute(byte)` → `\n`, `\r`, `\t`, backspace
- `csi_dispatch(...)` → movimiento de cursor (`ESC[H`, `ESC[A`...), borrado (`ESC[2J`,
  `ESC[K`), colores SGR (`ESC[31m`...)

Empezá por el subset mínimo: print + newline + CR + SGR de colores + clear. `ls --color`
andando es el primer milestone; `vim` es el boss final (necesita alternate screen y más CSI).

Lecturas: docs del crate `vte`, **vt100.net** (referencia histórica),
**"XTerm Control Sequences"** (invisible-island.net/xterm/ctlseqs/ctlseqs.html — LA
referencia de escape codes), y el código de `alacritty_terminal` como referencia cuando
no sepas qué hace una secuencia.

**Verificación**: tests unitarios — le das bytes al parser y assertás el estado de la
grilla (ej: `b"hola\r\n"` → fila 0 dice "hola", cursor en (0,1)). Acá el testing es
HERMOSO porque es una máquina de estados pura, sin IO.

### Fase 4 — Un terminal en pantalla (Rust + egui) (~1 semana)

**Objetivo**: app `eframe` que muestra UN terminal usable en una ventana normal (sin
canvas todavía).

- Thread lector: `pty.read()` en loop → `mpsc::Sender<Vec<u8>>` → el estado del terminal
  (detrás de `Mutex`) → `ctx.request_repaint()`.
- Render: pintar la grilla con `Painter::text()` usando fuente monoespaciada
  (`TextStyle::Monospace`), celda por celda con sus colores.
- Input: `egui::Event::Text` y `Event::Key` → traducir a bytes (Enter → `\r`,
  flecha arriba → `ESC[A`, Ctrl+C → byte `0x03`) → `pty.write()`.
- Resize: cambió el tamaño del widget → recalcular cols/rows → `pty.resize()` (el kernel
  manda SIGWINCH al shell solo).

Lecturas: egui docs (docs.rs/egui), ejemplos de eframe
(github.com/emilk/egui/tree/main/examples), y el código de `egui_term`
(github.com/Harzu/egui_term) como referencia de cómo otro resolvió exactamente esto.

**Verificación**: correr `htop` y `vim` dentro de tu ventana.

### Fase 5 — EL PIZARRÓN: canvas infinito con N terminales (~1 semana)

**Objetivo**: lo que soñaste.

- `egui::Scene` como contenedor pan/zoom; estado: `Vec<TerminalNode { id, rect, terminal }>`.
- Cada nodo: barra de título (para arrastrar) + el widget terminal de la fase 4.
- Botón "+" spawnea un PTY nuevo; cerrar nodo → `Drop` mata el shell (fase 2 pagando dividendos).
- Foco: UN nodo tiene foco de teclado; click para enfocar.
- Gotcha conocido: separar drag-de-nodo vs interacción-dentro-del-terminal, y scroll
  del mouse (¿panea el canvas o scrollea el terminal?) — resolver con hover + foco.
- Gotcha de zoom: texto renderizado escala solo en egui (ventaja de immediate mode),
  pero con zoom muy bajo conviene degradar a rectángulos de color (LOD) por performance.

**Verificación**: 5+ terminales a la vez, una corriendo `htop`, otra `cargo build`, pan y
zoom fluidos entre ellas.

### Fase 6 — Polish (opcional, sin orden)

Scrollback, selección + copiar, temas de color, persistencia del layout (serde + JSON),
links clickeables, agrupar nodos.

---

## Estructura final del workspace

```
pizarron/
├── Cargo.toml            # workspace
├── libpty/               # C: pty.c, pty.h
├── crates/
│   ├── pty-sys/          # FFI crudo (build.rs con `cc`)
│   ├── pty/              # wrapper seguro (RAII)
│   ├── term-model/       # vte + grilla (lógica pura, testeable)
│   └── pizarron/         # binario eframe: canvas + nodos
```

Dependencias totales: `cc`, `vte`, `eframe/egui`, `libc` — y nada más. Poquitas a
propósito: cada dependencia que no agregás es una capa que entendés.

## Reglas del proyecto (para no perder el norte)

1. **No avanzar de fase sin que la verificación de la anterior pase.** htop es el juez.
2. **Cuando te trabes**: primero el man page / la referencia de la fase, después el código
   de `alacritty_terminal` o `egui_term`, y recién después preguntar.
3. Si la fase 3 te consume más de 2-3 semanas, swap a `alacritty_terminal` sin culpa:
   el aprendizaje de C/FFI/GUI sigue intacto.

## Verificación end-to-end del proyecto completo

Abrir el pizarrón, crear 3 terminales: en una `vim` (editar y guardar un archivo), en otra
`htop` (quit con q), en la tercera `ls --color` + `cargo build` de un proyecto real.
Pan/zoom mientras todas corren. Cerrar un nodo y verificar con `ps` que el shell hijo murió.
