# El motor 3D: geometría con profundidad y billboards

Segundo backend de dibujo de VimMon, hermano del framebuffer de
`plugins/renderer/`. Vive en `plugins/renderer3d/` y corre sobre **SDL_GPU**,
la API de GPU de SDL3.

## Probarlo

```bash
make example3d      # compila C y shaders
./build/hello_3d    # WASD y flechas para moverse, ESC para salir
```

Necesita un driver Vulkan instalado. Ver [Requisitos](#requisitos) al final:
si falta, el motor avisa por `stderr` y no arranca.

## Los límites son el diseño

El motor hace exactamente **dos** cosas:

1. **Geometría con profundidad** — cubos y planos texturados. El mundo.
2. **Billboards** — sprites que siempre encaran a la cámara. Los enemigos,
   los ítems, todo lo que "vive" en el mundo.

No hay mallas arbitrarias, ni sombras, ni PBR, ni animación esquelética. Es el
motor de DOOM y Duke Nukem 3D, no el de un mundo abierto.

Esa limitación no es pobreza: es lo que permite que una sola persona entienda
el motor entero, y —como se ve más abajo— es lo que **habilita** las soluciones
simples que un motor grande no puede usar.

## Dónde encaja

`renderer3d.h` es un vtable, igual que `renderer.h`. No incluye SDL a propósito.
Los dos contratos conviven porque en SDL3 la API 2D y la de GPU comparten el
mismo `SDL_GPUDevice` (ver `SDL_PROP_RENDERER_GPU_DEVICE_POINTER` en
`SDL_render.h`): una ventana, un device, las dos formas de dibujar encima.

| Contrato | Backend | Para qué |
|----------|---------|----------|
| `plugins/renderer/renderer.h` | `sdl_fb.c` (SDL_Render) | 2D a mano, píxel por píxel |
| `plugins/renderer3d/renderer3d.h` | `gpu_sdl.c` (SDL_GPU) | 3D con GPU y shaders |

## Pixel art de verdad

Todo se dibuja a una resolución **interna** chica —por defecto 480×270— y recién
al final se estira a la ventana con filtro `NEAREST`.

```
  480×270  ──── SDL_BlitGPUTexture(NEAREST) ────▶  960×540
  (lienzo interno)                                 (ventana)
```

Dos cosas gratis:

- Los píxeles son cuadrados grandes y parejos. No un desenfoque que cambia de
  tamaño según cuán grande abriste la ventana.
- La GPU pinta ~8 veces menos píxeles que a 1080p.

El `SDL_BlitGPUTexture` con `NEAREST` evita tener que escribir un pipeline de
pantalla completa solo para escalar.

## Cómo se dibuja un frame

```
frame_begin()   → pide un command buffer
                → arma vista×proyección y los ejes de la cámara
                → abre un render pass contra el lienzo CHICO
dibujar_*()     → agrega órdenes al pass
frame_end()     → cierra el pass
                → blitea el lienzo estirado a la ventana
                → envía el command buffer entero de una
```

La diferencia con `SDL_Render`: ahí decías "dibujá esto" y pasaba. Acá **armás
una lista de órdenes** y después la mandás. Es más verboso, y a cambio la GPU
recibe todo junto en vez de que la CPU la interrumpa mil veces.

## El truco del billboard

La idea ingenua es rotar el sprite para que apunte a la cámara: hay que calcular
un ángulo, y si la cámara se mueve el ángulo queda viejo.

El shader hace lo contrario — **no rota nada**:

```glsl
vec3 p = b.centro.xyz
       + b.derecha.xyz * (in_pos.x * b.centro.w)
       + b.arriba.xyz  * (in_pos.y * b.derecha.w);
```

Arma el cuadrado usando los ejes de la cámara como lados. Si los lados del
cuadrado **son** los ejes de la pantalla, el cuadrado es paralelo a la pantalla
por construcción. No puede no mirarte.

La CPU manda los dos ejes ya calculados (`gpu_frame_begin`), y los manda en
modo **cilíndrico**: el "derecha" de la cámara aplanado contra el piso y el
"arriba" del mundo. Así el sprite gira siguiéndote pero nunca se acuesta, ni
aunque mires al techo. Es como se paran los enemigos de DOOM.

## Recorte por alfa, no mezcla

Los billboards usan `discard`, no alpha blending. Es una decisión, no un atajo.

Mezclar obliga a dibujar los sprites de atrás hacia adelante, **ordenados por
distancia, cada frame**. Ordenar 200 enemigos por frame cuesta, y si te
equivocás aparecen halos y recortes raros.

Con `discard` el píxel transparente simplemente **no existe**: no escribe color
ni profundidad. El z-buffer resuelve el orden solo, gratis, y se puede dibujar
en cualquier orden.

Se puede hacer porque el pixel art no tiene transparencias suaves: un píxel
está, o no está. **La limitación del estilo es la que habilita la solución
simple.** Un motor AAA no puede usar este atajo.

## La trampa de los bindings

SDL_GPU exige *sets* fijos para SPIR-V:

| Etapa | Texturas | Uniforms |
|-------|----------|----------|
| vertex | `set = 0` | `set = 1` |
| fragment | `set = 2` | `set = 3` |

Poner otro número **no da error**. Compila, linkea, corre, y la pantalla queda
negra. Es el peor tipo de bug: el que no se queja. Por eso está escrito arriba
de cada shader.

## Detalles que parecen arbitrarios y no lo son

- **El cubo tiene 24 vértices, no 8.** Cada cara necesita *sus* coordenadas de
  textura y *su* normal, y un vértice compartido entre tres caras no puede
  tener tres UV ni tres normales.
- **La normal viaja como atributo del vértice**, no se deduce. La primera
  versión la sacaba de `normalize(in_pos)` para ahorrar 12 bytes, y estaba mal
  en los dos casos que el motor dibuja: en un plano XZ todos los vértices
  tienen `y = 0`, así que la componente Y nunca gana y el piso quedaba con la
  normal horizontal; y en un cubo las tres componentes valen lo mismo (0.577),
  así que "cuál es mayor" elegía siempre la misma. Todo salía con un solo tono.
  Ser vivo salió caro.
- **La normal se rota con el modelo, no con la MVP.** Una normal dice hacia
  dónde *mira* una cara, y eso no cambia según dónde esté la cámara. Para
  escalas no uniformes lo correcto sería la inversa traspuesta; con cubos y
  planos alineados a los ejes, normalizar después alcanza.
- **La matriz de perspectiva usa rango de profundidad [0,1]**, convención de
  Vulkan y D3D, no el [-1,1] de OpenGL. Copiar una matriz de un tutorial de
  OpenGL te deja todo recortado mal.
- **El formato de profundidad se consulta**, no se asume: `SDL_GPUTextureSupportsFormat`
  prueba D24 → D32F → D16 porque no todos los drivers soportan lo mismo.
- **Sin descarte de caras traseras** (`CULLMODE_NONE`): los billboards son planos
  y con culling se verían de un solo lado.

## Requisitos

SDL_GPU necesita un **ICD** (el driver Vulkan). Tener `vulkan-icd-loader` no
alcanza: el loader es la centralita, no el driver.

```bash
sudo pacman -S vulkan-dzn vulkan-swrast
vulkaninfo --summary          # verificar que quedó
```

- `vulkan-dzn` — Mesa Dozen: traduce Vulkan a D3D12 y usa la GPU real vía WSLg
  (requiere `/dev/dxg`, que en WSL2 está).
- `vulkan-swrast` — lavapipe: Vulkan por software. Lento, pero funciona siempre
  y sirve para descartar que un problema sea del driver:

```bash
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json ./build/hello_3d
```

## Verificar sin pantalla

Con GPU un binding mal puesto **no da error**: da pantalla negra. "No crashea"
no es "dibuja bien". Por eso el backend expone una captura:

```c
int gpu_sdl_capturar_bmp(const char *ruta);   // llamar DESPUÉS de frame_end()
```

Baja el lienzo interno de la GPU a RAM y lo guarda. No está en el vtable a
propósito: es diagnóstico del backend, no algo que un juego use.

```bash
./build/hello_3d --captura /tmp/frame.bmp
```

Necesita una **fence**: la CPU y la GPU corren en paralelo y `submit` no
espera, así que sin `SDL_WaitForGPUFences` se leería el buffer antes de que la
GPU lo llene y saldría basura.

Después se puede medir con números en vez de mirar. Ejemplos reales que se
usaron para validar este motor:

| Qué | Cómo se comprueba |
|-----|-------------------|
| Perspectiva y billboards | Un billboard de 1×1 a 10 unidades con FOV 70 tiene que dar 19,3 px de lado en un lienzo de 270 de alto. Dio 20×20, relación 1,000. |
| Iluminación | Contar colores distintos. Si el shader tiene 3 escalones y aparece uno solo, las normales están mal. |

Ojo con medir sprites que se solapan: un *bounding box* de todos los píxeles
rojos junta dos enemigos y da una relación falsa. Hay que aislar componentes
conexas, o mejor, hacer un test con **un** objeto y tamaño conocido.

## Lo que todavía no hace

- **No está conectado al bus ni a PAED.** `escena.h` ya tiene `SceneState` con
  cuerpos, luces y cámara, y `scene_view.c` los proyecta a mano en el
  framebuffer. Falta un `scene_view3d.c` que dibuje ese mismo `SceneState` con
  este backend. El modelo de datos no cambia: cambia quién lo pinta.
- **No hay carga de imágenes.** Las texturas se arman en memoria
  (`textura_desde_pixeles`). Falta enchufar SDL3_image.
- **Las UV no se repiten.** El piso es un plano con UV de 0 a 1, así que una
  textura de 16×16 se estira sobre las 40 unidades enteras en vez de repetirse
  como baldosas. Falta un factor de repetición por objeto.
- **Las luces de `escena.h` no iluminan.** El shader tiene una luz direccional
  fija de 3 escalones, hardcodeada en `gpu_dibujar_malla`.
- **No hay switch de backend en runtime.** Hoy se elige en tiempo de compilación.
