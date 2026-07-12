---

kanban-plugin: board

---

## Backlog

- [ ] Instalar libsdl2-dev #fase3
- [ ] `plugins/renderer/renderer.h` — interfaz abstracta de render #fase3
- [ ] `plugins/renderer/sdl_fb.c` — ventana SDL2 + textura como framebuffer #fase3
- [ ] Primitivas a mano: put_pixel, línea (Bresenham), rectángulo #fase3
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


## Hecho

- [x] Limpiar directorios → _void #fase0
- [x] Crear estructura de carpetas del OS #fase0
- [x] `bus/plugin.h` — el contrato que nunca cambia #fase0
- [x] `bus/bus.c` + `bus/bus.h` — el bus de plugins #fase0
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




%% kanban:settings
```
{"kanban-plugin":"board","show-checkboxes":true,"tag-colors":[],"move-tags":true}
```
%%