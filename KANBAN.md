---

kanban-plugin: board

---

## Backlog

- [ ] Conectar EVENT_KEYBOARD/EVENT_MOUSE a un consumidor real (hoy solo hay debug prints en `plugins/input/input.c`) #fase3
- [ ] Conectar el mundo 2D del motor (`Entity`/`World`) con el `SceneState` de PAED — hoy son dos mundos separados #fase3
- [ ] Dibujar cubo desde PAED (proyección 3D→2D simple, sin GPU) #fase3
- [ ] Split screen: panel izquierdo (AI chat) + panel derecho (viewport) #fase3
- [ ] Test: scene.paed con cubo → aparece en pantalla #fase3
- [ ] `plugins/raycaster/map.c` — mapa como grilla 2D hardcodeada #fase3b (no arrancar hasta tener renderer.h + put_pixel de FASE 3)
- [ ] `plugins/raycaster/ray.c` — DDA: un rayo por columna, paredes sólidas de color #fase3b
- [ ] Input por el bus: mover/rotar jugador + colisión contra la grilla #fase3b
- [ ] Texturas en paredes (BMP con SDL o stb_image) #fase3b
- [ ] Z-buffer por columna + sprites billboard (enemigo 2D estilo boomer shooter) #fase3b
- [ ] Test: caminar un mapa con un enemigo sprite que te mira #fase3b
- [ ] `plugins/monitor/monitor.c` — leer /proc/meminfo y /proc/stat #fase4
- [ ] Mostrar RAM, CPU, plugins activos en un panel #fase4
- [ ] Registrar en el bus (EVT_MONITOR_TICK) #fase4
- [ ] Mover/adaptar PseudoGames como plugin en `plugins/ide/` #fase5
- [ ] Conectar al bus: teclado, render, AI #fase5
- [ ] Editor PAED integrado al split screen #fase5
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
- [x] `docs/paed_spec.md` — spec del lenguaje PAED v1.0 #fase0
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
- [x] SDL2 ya instalado (`sdl2-compat`, headers en `/usr/include/SDL2/`) — card "Instalar libsdl2-dev" satisfecha #fase3
- [x] `plugins/renderer/renderer.h` — interfaz abstracta: vtable de punteros a función, sin tipos SDL (backend intercambiable) #fase3
- [x] `plugins/renderer/sdl_fb.c` — backend framebuffer: ventana SDL2 + textura streaming ARGB8888, framebuffer privado (`static uint32_t *pixels`) #fase3
- [x] Primitivas a mano: `put_pixel` (con guard de límites), `fill_rect`, `draw_line` (Bresenham, solo enteros) #fase3
- [x] `engine/engine.{h,c}` — motor 2D de entidades: `Entity` con callbacks `update`/`draw`, `World` (pool estático), game loop dueño del tiempo (`dt`, ~60fps), colisión AABB (`entity_overlaps`) #fase3
- [x] `game/game.{h,c}` — seam del juego del usuario: `game_setup(World*)` donde vive el juego (starter: cuadrado movible con flechas/WASD) #fase3
- [x] `examples/hello_entity.c` — plantilla ejecutable (rectángulo que rebota) #fase3
- [x] Consola de comandos en `main.c` (`engine`/`scene`/`ai`/`help`/`quit`); "engine" publica `EVENT_RENDER_FRAME` → el renderer lanza el motor con `game_setup` #fase3
- [x] `plugins/renderer/renderer.c` — puente bus↔motor: `on_event(EVENT_RENDER_FRAME)` abre el motor y le cede el loop #fase3




%% kanban:settings
```
{"kanban-plugin":"board","show-checkboxes":true,"tag-colors":[],"move-tags":true}
```
%%