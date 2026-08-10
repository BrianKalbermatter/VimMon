---

kanban-plugin: board

---

## Backlog

- [ ] Parser PAED: una instrucción no puede partirse en dos líneas — el parser lee línea por línea. Un `ESCRIBIR` largo hay que dejarlo en una sola #fase2
- [ ] Parser PAED: `SEGUN` — CONFLICTO a decidir antes de implementar #fase2
- [ ] Parser PAED: `REPETIR`/`HASTA` — cero apariciones reales en el corpus, solo declaradas en `sintaxis.json`. Confirmar que existan antes de implementarlas #fase2
- [ ] Evaluador: guardar un árbol de la expresión en vez de re-parsear el texto en CADA vuelta del bucle. Hoy es simple y correcto, pero un `MIENTRAS` largo paga el costo en cada iteración #fase2
- [ ] Evaluador: usar el `AMBIENTE` para chequear tipos. Hoy se parsea pero no se usa: asignarle un texto a algo declarado `ENTERO` no da error #fase2
- [ ] Evaluador: los arreglos no chequean el TIPO declarado. `A: ARREGLO[1..5] DE ENTERO` acepta que le metan un texto en `A[2]`, igual que pasa con los escalares #fase2
- [ ] Evaluador: `NFDS`/`FDS` necesitan SECUENCIAS, que el intérprete no tiene. Hoy avisan en vez de inventar un valor #fase2
- [ ] Avisar cuando se usa `==`: ya está resuelto que NO existe en AED (`TEORIA_COMPLETA.txt:324` define `=`, y la wiki lo marca como error de escritura arrastrado, 91 usos). Hoy se acepta callado para no romper los archivos; debería avisar sin frenar la ejecución #fase2
- [ ] Confirmar contra la cátedra: `-2 ** 2` da 4 porque la tabla de prioridad pone los unarios ARRIBA de la potencia. En casi todos los lenguajes da -4. ¿Es lo que quiere AED? #fase2
- [ ] Parser PAED: `FUNCION`/`PROCEDIMIENTO` anidados dentro de `AMBIENTE` #fase2
- [ ] Parser PAED: nombre de `ACCION` con espacios (`ACCION Ejercicio de Parcial ES`) #fase2
- [ ] Parser PAED: declaración múltiple `A,B,SUMA: entero`. Es la forma del único ejemplo con autoridad de cátedra (`AED_2021_UnI.pdf:10`) y hoy da "nombre de variable invalido" #fase2
- [ ] Parser PAED: `VARIABLES` como sub-sección de `AMBIENTE`. Aparece en `AED_2021_UnI.pdf:10` y en ninguna otra fuente. Antes de implementar, confirmar si es obligatoria #fase2
- [ ] Decidir el `;`: la cátedra lo usa como SEPARADOR (la última sentencia no lo lleva) y el parser lo exige como terminador. Cambiarlo rompe todos los `.paed` del repo — decidir antes de tocar #fase2
- [ ] Interprete PAED: implementar `ARR`/`AVZ`/`CREAR`/`CERRAR`/`LEER` (hoy parsean pero no ejecutan) #fase2
- [ ] Corroborar contra la wiki: `RETORNAR`, `TRUNC`, `ABSO`, `REDOND` — 0 apariciones en los apuntes #fase2
- [ ] Conectar EVENT_KEYBOARD/EVENT_MOUSE a un consumidor real (hoy solo hay debug prints en `plugins/input/input.c`) #fase3
- [ ] Recortar contra el plano cercano: hoy `scene_view.c` acota las coordenadas de las esquinas en vez de generar vértices en el borde, y un objeto que abraza la cámara (el suelo) queda mal #fase3
- [ ] Z-buffer por píxel: hoy el orden del pintor usa la profundidad del CENTRO, así que dos cuerpos que se cruzan se ordenan mal #fase3
- [ ] Caras del cubo con sombreado: hoy se rellena el rectángulo que contiene las 8 esquinas, no las 6 caras #fase3
- [ ] `LUZ` que ilumine de verdad: hoy solo se dibuja una cruz donde está #fase3
- [ ] Split screen: panel izquierdo (AI chat) + panel derecho (viewport) #fase3
- [ ] `plugins/raycaster/map.c` — mapa como grilla 2D hardcodeada #fase3b (no arrancar hasta tener renderer.h + put_pixel de FASE 3)
- [ ] `plugins/raycaster/ray.c` — DDA: un rayo por columna, paredes sólidas de color #fase3b
- [ ] Input por el bus: mover/rotar jugador + colisión contra la grilla #fase3b
- [ ] Texturas en paredes (BMP con SDL o stb_image) #fase3b
- [ ] Z-buffer por columna + sprites billboard (enemigo 2D estilo boomer shooter) #fase3b
- [ ] Test: caminar un mapa con un enemigo sprite que te mira #fase3b
- [ ] `plugins/monitor/monitor.c` — leer /proc/meminfo y /proc/stat #fase4
- [ ] Mostrar RAM, CPU, plugins activos en un panel #fase4
- [ ] Registrar en el bus (EVT_MONITOR_TICK) #fase4
- [ ] Mover/adaptar `paed/` como plugin en `plugins/ide/` #fase5
- [ ] Conectar al bus: teclado, render, AI #fase5
- [ ] Editor PAED integrado al split screen (hoy `edit` abre PseudoGames en su propia ventana; falta el split dentro de VimMon) #fase5
- [ ] `kernel/boot/entry.asm` — Multiboot2 + request framebuffer #fase6
- [ ] `kernel/drivers/fb.c` — framebuffer VESA #fase6
- [ ] `kernel/drivers/font.c` — PSF font embebida #fase6
- [ ] `kernel/kernel/main.c` — kmain() #fase6
- [ ] `kernel/arch/x86_64/gdt.c` — GDT #fase6
- [ ] `kernel/arch/x86_64/idt.c` — IDT #fase6
- [ ] `kernel/mm/pmm.c` — bitmap allocator #fase6
- [ ] `kernel/mm/heap.c` — kmalloc/kfree #fase6
- [ ] `kernel/drivers/keyboard.c` — PS/2 IRQ1 #fase6
- [ ] `kernel/drivers/timer.c` — PIT 100Hz #fase6
- [ ] Boot en QEMU: VimMon arranca sin Linux #fase6
- [ ] Instalar vulkan-headers + vulkan-validation-layers + vulkan-dzn #futura (no arrancar hasta cerrar FASE 3 — mismo `renderer.h`, el framebuffer SDL2 queda como fallback)
- [ ] `plugins/renderer/vulkan_init.c` — instancia + device + swap chain #futura
- [ ] `plugins/renderer/vulkan_backend.c` — dibujar triángulo (hola Vulkan) #futura
- [ ] `plugins/renderer/vulkan_backend.c` — dibujar cubo desde PAED #futura
- [ ] Switch de backend en runtime: framebuffer ↔ Vulkan #futura


## En progreso

- [ ] Aprendiendo C
- [ ] Crear el primer juego en `game/game.c` (usando el motor 2D)


## Hecho

- [x] Limpiar directorios → _void #fase0
- [x] Crear estructura de carpetas del OS #fase0
- [x] `bus/plugin.h` — contrato del bus: `Plugin`, `EventType` (con `EVENT_MOUSE` y centinela `EVENT_COUNT`), `PLUGIN_MAX` #fase0
- [x] `bus/bus.c` — bus real: `bus_init`, `bus_register` (un plugin puede suscribirse a varios `EventType`), `bus_send`, `bus_unregister` (swap-con-el-ultimo), `bus_shutdown` (deduplicado) #fase0
- [x] Loop principal en `main.c` con manejo de `SIGINT` (Ctrl+C apaga limpio, no mata el proceso) #fase0
- [x] `plugins/input/input.c` — plugin de teclado + mouse: modo raw de terminal (termios), lectura no bloqueante, reporte SGR de mouse (click + posición), restaura la terminal al apagar #fase0
- [x] `paed/Frankly/docs/PAED.md` + `data/sintaxis.json` — spec y definicion formal de PAED, fuente unica de verdad #fase0
- [x] PAED.md v4.0 absorbe entero `docs/paed_spec.md` (v2.0), que se borra: una sola spec, no dos. Se separan **catedra** / **decidido** / **implementado**, porque habia decisiones documentadas que el parser no cumplia (keywords case-insensitive, `;` como separador) #fase0
- [x] `edit` abre **PseudoGames entero** — el IDE completo con su menu, niveles, wiki, pomodoro y editor adentro. Es una opcion del OS como `engine` o `ai`, no un programa que se corre por afuera #fase5
- [x] `plugins/editor/editor.c` escucha `EVENT_EDITOR_OPEN` y lanza `paed/aed`. main.c no sabe QUE programa es ni donde vive: el dia que haya otro, cambia el plugin y nada mas #fase5
- [x] Si `paed/aed` no esta compilado, el plugin corre `make -C paed` solo. La primera vez que alguien escribe `edit` no tiene por que saber que habia que compilar a mano #fase5
- [x] PseudoGames corre parado en `paed/`, porque carga `assets/`, `data/` y `saves/` con rutas relativas. `VIMMON_IDE` permite cambiar el programa sin recompilar VimMon #fase5
- [x] REGISTRO implementado — `vector2 = REGISTRO ... FIN_REGISTRO` en el AMBIENTE, y `pori.vx` como destino y dentro de expresiones. Es el `struct` de C con otro nombre #fase2
- [x] `AlgebraRectas/recta.paed` corre ENTERO: de 10 errores a 0. Verificado a mano que el calculo da bien — P0=(0,0), D=(1,-4), dominio [-4,4] produce el segmento (-4,16) a (4,-16), que es la recta y=-4x #fase2
- [x] Los registros se APLANAN: `pori` de tipo vector2 se guarda como las variables "pori.vx" y "pori.vy". El Entorno no sabe nada de registros. El precio es que no se puede asignar un registro entero (`p1 := p2`), que no aparece en el corpus #fase2
- [x] Un campo que el registro NO declara se rechaza (`'p' no tiene un campo 'vz'`). Sin ese chequeo el registro no serviria de nada: como los campos se aplanan, `p.vz` naceria solo en su primera asignacion igual que un escalar #fase2
- [x] Keywords case-insensitive RESUELTO — `accion`, `MiEnTrAs` y `FiN_sI` parsean igual que en mayusculas. Alcanza a palabras clave, tipos, nombres de procedimiento y claves de parametro. Los IDENTIFICADORES siguen distinguiendo: `total` y `Total` son dos variables. Regla: lo que define el lenguaje no distingue, lo que nombras vos si #fase2
- [x] Verificado que ninguna variable del corpus real choca solo por mayusculas, asi que mantener los identificadores case-sensitive no rompe ningun `.paed` existente #fase2
- [x] Bug encontrado al hacerlo: `FIN_MIENTRAS` y `FIN_PARA` se distinguian por `linea[4] == 'P'`. Con `fin_para` en minuscula eso es `'p'` y el bucle se cerraba como si fuera un `FIN_MIENTRAS` #fase2
- [x] Cierre de la `ACCION` RESUELTO — era el ultimo punto BLOQUEANTE de la spec. Se aceptan `FIN_ACCION` y `FINACCION`: las dos son una sola palabra, cuestan un `strcmp` y ningun lookahead. `FACCION` se rechaza (abreviar FIN a F deja el cierre incompleto) y `FIN ACCION` con espacio tambien, aunque sea la forma de la catedra: partida en dos obliga a mirar la palabra siguiente. Las tres formas rechazadas igual CIERRAN el bloque, para no cascar un error por cada linea que venga despues #fase2
- [x] Verificado de primera mano `AED_2021_UnI.pdf` pagina 10: la catedra efectivamente escribe `FIN ACCION` con espacio. La cita que venia arrastrada de `paed_spec.md` era correcta — es una captura de Sublime Text 2 dentro del apunte, no un BNF formal #fase2
- [x] `docs/plugin_spec.md` — cómo crear plugins #fase0
- [x] `KANBAN.md` — este archivo #fase0
- [x] `Makefile` raíz — compila bus + plugins juntos #fase0
- [x] `main.c` raíz — entry point, inicializa bus y plugins #fase0
- [x] `plugins/ai/ai.h` — interfaz del plugin AI #fase1
- [x] `plugins/ai/ai.c` — HTTP client a Ollama con libcurl #fase1
- [x] `plugins/ai/ai.c` — parser de respuesta JSON (cJSON) #fase1
- [x] `plugins/ai/ai.c` — armar el prompt con scene.paed como contexto #fase1
- [x] `plugins/ai/ai.c` — registrar en el bus (EVT_AI_REQUEST → EVT_AI_RESPONSE) #fase1
- [x] Test: mandar "creá un cubo rojo" → recibir PAED delta en terminal #fase1
- [x] `plugins/ide/paed/parser.h` + `parser.c` — leer scene.paed línea por línea #fase2
- [x] `plugins/ide/paed/interpreter.h` + `interpreter.c` — ejecutar comandos PAED #fase2
- [x] `plugins/ide/scene.paed` — archivo de estado de la escena #fase2
- [x] Integrar: AI response → append a scene.paed → re-parsear #fase2
- [x] Test: scene.paed con un cubo → interprete lo lee sin crash #fase2
- [x] PAED = pseudocodigo AED puro: `data/sintaxis.json` solo tiene lo corroborado contra `wiki.txt`, `TEORIA_COMPLETA.txt` y los `.paed` de la catedra #fase2
- [x] La escena 3D salio del lenguaje: es una libreria aparte en `data/escena.json`, se carga ademas de `sintaxis.json` #fase2
- [x] `paed/Frankly/tools/generar.sh` — genera `paed.tmLanguage.json` y `core/palabras.sh` desde `sintaxis.json` (se acabo copiar keywords a mano) #fase2
- [x] `plugins/ide/parser.c` — parser de PAED real (ACCION/AMBIENTE/PROCESO) que reporta errores con archivo:linea y NUNCA ignora en silencio #fase2
- [x] Pila de bloques en el parser: `SI`/`SINO`/`FIN_SI` y `MIENTRAS`/`FIN_MIENTRAS` anidados. Una variable no puede representar un CAMINO de anidamiento, y es pila porque los bloques cierran en orden inverso al que se abren #fase2
- [x] Saltos parcheados al cerrar el bloque (backpatching): cada `SI`/`MIENTRAS` guarda a dónde ir, igual que el bytecode. `instrs[]` sigue siendo plano #fase2
- [x] Asignación `:=` con destino simple: se parsea y se guarda la expresión cruda (todavía sin evaluador) #fase2
- [x] Errores de bloque que citan la línea de APERTURA: "FIN_SI cierra un MIENTRAS abierto en la linea 3", "falta FIN_MIENTRAS: el MIENTRAS de la linea 3 quedo abierto" #fase2
- [x] Test: `ejercicio2_1_11.paed` (MIENTRAS > MIENTRAS > SI/SINO) parsea entero y cada `FIN_MIENTRAS` vuelve al suyo #fase2
- [x] `plugins/ide/expr.{h,c}` — evaluador de expresiones por descenso recursivo. La prioridad NO es una tabla de números: es el orden en que las funciones se llaman entre sí (`TEORIA_COMPLETA.txt:361-371`) #fase2
- [x] Cortocircuito en `Y` y `O`, que la teoría exige textualmente ("En AND, si el primer operando es Falso, el segundo no se evalúa"). Cambia el comportamiento, no solo la velocidad #fase2
- [x] Tabla de variables (`Entorno`) sin malloc, y `ESCRIBIR` que EVALÚA sus argumentos: antes `ESCRIBIR(cont_pal)` imprimía el nombre en vez del valor #fase2
- [x] `interp_exec` sigue los saltos con un índice en vez de recorrer el array: es un contador de programa. `SI`/`SINO`/`MIENTRAS`/`PARA` se ejecutan de verdad #fase2
- [x] Guarda de bucle infinito (2M pasos): el intérprete corre DENTRO del game loop, así que un programa colgado colgaba la ventana entera #fase2
- [x] Bug: `parse_instruction` buscaba el `=` de `clave = valor` con `strchr`, sin respetar comillas, y `ESCRIBIR("a = b")` quedaba destrozado #fase2
- [x] Test: programa con `PARA`, `PARA` en reversa, `MIENTRAS` acumulador, `SI/SINO` y `PARA` anidado corre entero y da los valores correctos #fase2
- [x] `build/paedrun` — arnés que corre un `.paed` en la terminal, sin SDL ni bus. El intérprete vive dentro del game loop, así que probar el lenguaje era abrir la ventana y mirar; un test que hay que mirar no es un test #fase2
- [x] `make test` — corre todos los `.paed` de `paed/Frankly/tests/` y compara contra el `.esperado` de al lado. Agregar un test es dejar los dos archivos: no hay lista que mantener a mano #fase2
- [x] Evaluador: `ARREGLO[desde..hasta] DE <tipo>` en el `AMBIENTE`, `A[i]` en expresiones y como destino. El índice es una expresión completa, así que `A[(izq+der) DIV 2]` sale gratis. Los límites se chequean en runtime: `indice 4 fuera de rango: 'A' va de 5 a 9` #fase2
- [x] Algoritmos reales corriendo: búsqueda lineal, búsqueda binaria, burbuja con `PARA` anidado, Euclides, primos, Fibonacci, factorial #fase2
- [x] Bug: `falla(c, "%s", c->env->error)` pasaba el buffer de error como argumento de un `vsnprintf` que escribe en ESE MISMO buffer. Aliasing: el mensaje llegaba vacío (`error: ` pelado). Se copia antes de pasarlo #fase2
- [x] Bug: `stdout` con buffer y `stderr` sin él descolocaban el orden de la salida al mandarla a una tubería, y los errores aparecían antes de líneas impresas primero. `setvbuf` en `paedrun` #fase2
- [x] `PARA <var> := <desde> HASTA <hasta>[; <paso>] HACER` / `FIN_PARA`. El paso es OPCIONAL y por defecto 1, corroborado en `TEORIA_COMPLETA.txt:565-571` ("Si el incremento es distinto de 1, debe indicarse"). En reversa se usa paso negativo, no una palabra tipo `downto`. `recta.paed:47` decía `a` en vez de `HASTA` y se corrigió #fase2
- [x] SDL2 ya instalado (`sdl2-compat`, headers en `/usr/include/SDL2/`) — card "Instalar libsdl2-dev" satisfecha #fase3
- [x] `plugins/renderer/renderer.h` — interfaz abstracta: vtable de punteros a función, sin tipos SDL (backend intercambiable) #fase3
- [x] `plugins/renderer/sdl_fb.c` — backend framebuffer: ventana SDL2 + textura streaming ARGB8888, framebuffer privado (`static uint32_t *pixels`) #fase3
- [x] Primitivas a mano: `put_pixel` (con guard de límites), `fill_rect`, `draw_line` (Bresenham, solo enteros) #fase3
- [x] `engine/engine.{h,c}` — motor 2D de entidades: `Entity` con callbacks `update`/`draw`, `World` (pool estático), game loop dueño del tiempo (`dt`, ~60fps), colisión AABB (`entity_overlaps`) #fase3
- [x] `game/game.{h,c}` — seam del juego del usuario: `game_setup(World*)` donde vive el juego (starter: cuadrado movible con flechas/WASD) #fase3
- [x] `examples/hello_entity.c` — plantilla ejecutable (rectángulo que rebota) #fase3
- [x] Consola de comandos en `main.c` (`engine`/`scene`/`ai`/`help`/`quit`); "engine" publica `EVENT_RENDER_FRAME` → el renderer lanza el motor con `game_setup` #fase3
- [x] `plugins/renderer/renderer.c` — puente bus↔motor: `on_event(EVENT_RENDER_FRAME)` abre el motor y le cede el loop #fase3
- [x] `plugins/ide/scene_view.{h,c}` — el `SceneState` de PAED se dibuja en el motor: base de cámara (derecha/arriba/adelante con producto vectorial), perspectiva dividiendo por la profundidad, orden del pintor. Se monta como UNA entidad del pool, así `engine.c` no se tocó #fase3
- [x] Recarga en caliente de la ESCENA: `scene_view.c` vigila el mtime (en nanosegundos) de `scene.paed` cada ~0.5s y re-parsea sola. Si el archivo nuevo tiene errores, conserva la última escena buena #fase3
- [x] Renombrar el `Entity` de PAED a `Cuerpo`: chocaba con el `Entity` del motor, que son cosas distintas #fase3
- [x] Test: `scene.paed` con cubo → aparece en pantalla. Verificado con un `Renderer` falso que anota las llamadas: cubo de 1×1×1 a distancia 5 → rect de 115px centrado; a distancia 10 → 55px #fase3




%% kanban:settings
```
{"kanban-plugin":"board","show-checkboxes":true,"tag-colors":[],"move-tags":true}
```
%%