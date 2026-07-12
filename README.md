# VimMon

**Un sistema operativo embebido para programar desde la shell — construido desde cero, en C, para aprender de verdad.**

VimMon es mi proyecto de aprendizaje a largo plazo: en lugar de hacer tutoriales sueltos, estoy construyendo un sistema completo — bus de eventos, plugins, un intérprete de pseudocódigo, un renderer por software y, al final del camino, un kernel bare metal. Cada pieza existe para forzarme a entender un concepto de C o de sistemas a fondo.

> Regla de la casa: **yo escribo todo el código**. La IA es tutora, no autora.

## Cómo funciona

El corazón es un **bus de eventos** (`bus/`) donde cada componente se registra como plugin declarando qué eventos consume. Los plugins no se conocen entre sí — solo hablan a través del bus.

```
                        ┌─────────────┐
  EVENT_AI_REQUEST ───▶ │  plugin AI  │ ───▶ EVENT_AI_RESPONSE
                        └─────────────┘            │
                        ┌─────────────┐            ▼
                        │ plugin IDE  │ ◀── parsea PAED, actualiza escena
                        └─────────────┘
                        ┌─────────────┐
 EVENT_SCENE_UPDATE ──▶ │  renderer   │ ───▶ framebuffer SDL2 (píxel a píxel)
                        └─────────────┘
```

| Plugin | Qué hace | Estado |
|--------|----------|--------|
| `plugins/ai` | Cliente HTTP a Ollama (libcurl + cJSON); convierte lenguaje natural en comandos PAED | ✅ Funciona |
| `plugins/ide` | Parser e intérprete de **PAED**, el lenguaje de pseudocódigo del proyecto ([spec](docs/paed_spec.md)) | ✅ Funciona |
| `plugins/renderer` | Framebuffer SDL2 con primitivas a mano: put_pixel, Bresenham, proyección 3D→2D sin GPU | 🔨 En progreso |
| `plugins/monitor` | Lee `/proc/meminfo` y `/proc/stat`; panel de RAM, CPU y plugins activos | ⏳ Pendiente |

Cómo crear un plugin nuevo: [docs/plugin_spec.md](docs/plugin_spec.md).

## Compilar y correr

Requisitos: `clang`, `libcurl` (y [Ollama](https://ollama.com) corriendo local si querés el plugin AI).

```bash
make
./vimmon
```

## Subproyectos

Este repo es también mi cuaderno de trabajo — hay más de un proyecto adentro:

| Directorio | Qué es |
|------------|--------|
| [`PseudoGames/`](PseudoGames/) | Juego en C + SDL2 para aprender Algoritmos y Estructuras de Datos: editor con syntax highlighting propio, niveles, boss final y wiki integrada |
| `PseudoGames/Frankly/` | Tooling para PAED: ejercicios, stdlib, detección de sintaxis |
| [`docs/`](docs/) | Specs de PAED v1.0 y del sistema de plugins |
| `kernel/` | Primeros experimentos bare metal (fase futura) |
| `learnC/`, `CUDA/`, `bash/` | Ejercicios y apuntes del camino |

## Roadmap

El plan vive en [KANBAN.md](KANBAN.md). Resumen:

- [x] **Fase 0** — Bus de plugins y contratos
- [x] **Fase 1** — Plugin AI (Ollama + libcurl)
- [x] **Fase 2** — Intérprete PAED
- [ ] **Fase 3** — Renderer SDL2 framebuffer ← *acá estoy*
- [ ] **Fase 4** — Plugin monitor (`/proc`)
- [ ] **Fase 5+** — Kernel bare metal (userspace primero, kernel después)

## Licencia

Propietaria — ver [LICENSE](LICENSE). El código es visible con fines educativos, pero todos los derechos están reservados.
