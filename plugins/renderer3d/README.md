# Motor 3D de VimMon (SDL_GPU)

Backend 3D que rellena el contrato de `renderer3d.h`, hermano del backend 2D
de `plugins/renderer/`.

## Los limites son el diseno

Hace exactamente dos cosas:

1. **Geometria con profundidad** — cubos y planos texturados. El mundo.
2. **Billboards** — sprites que siempre encaran a la camara. Los enemigos.

Es el motor de DOOM y Duke Nukem 3D, no el de un mundo abierto. No hay
mallas arbitrarias, ni sombras, ni PBR, ni animacion esqueletica. Esa
limitacion es lo que permite que una persona lo entienda entero.

## Pixel art de verdad

Todo se dibuja a una resolucion interna chica (por defecto 480x270) y recien
al final se escala a la ventana con filtro `NEAREST`. Eso da pixeles cuadrados
parejos que no cambian de tamano segun cuan grande abriste la ventana, y de
paso la GPU pinta ~8 veces menos pixeles que a 1080p.

## Requisito: un driver Vulkan

SDL_GPU necesita un **ICD** (el driver Vulkan). Tener `vulkan-icd-loader`
instalado NO alcanza: el loader es la centralita, no el driver.

En WSL2 con `/dev/dxg` disponible:

```bash
sudo pacman -S vulkan-dzn vulkan-swrast
```

- `vulkan-dzn` — Mesa Dozen: traduce Vulkan a D3D12 y usa la GPU real via WSLg.
- `vulkan-swrast` — lavapipe: Vulkan por software. Lento, pero funciona siempre
  y sirve para descartar que un problema sea del driver.

Verificar que quedo:

```bash
vulkaninfo --summary
```

Si `SDL_CreateGPUDevice` sigue fallando con dzn, forzar el de software:

```bash
VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.x86_64.json ./build/hello_3d
```

## Compilar y correr

```bash
make example3d     # compila C y shaders (glslc: GLSL -> SPIR-V)
./build/hello_3d   # WASD y flechas para moverse, ESC para salir
```

Los `.spv` se leen en tiempo de ejecucion desde `plugins/renderer3d/shaders/`,
relativo al directorio desde donde arrancas. Se puede cambiar con
`-DRUTA_SHADERS='"otra/ruta"'`.

## Verificar sin pantalla

Con GPU un binding mal puesto no da error: da pantalla negra. "No crashea" no
es "dibuja bien".

```bash
./build/hello_3d --captura /tmp/frame.bmp
```

`gpu_sdl_capturar_bmp()` baja el lienzo interno de la GPU a RAM (con una fence,
porque `submit` no espera) y lo guarda. Detalle y ejemplos de medicion en
`docs/renderer3d.md`.

## Archivos

| Archivo | Que hace |
|---|---|
| `renderer3d.h` | El contrato. No incluye SDL a proposito. |
| `math3d.h/.c` | Mat4, V3, `mirar()` y `perspectiva()`. Nada mas. |
| `gpu_sdl.c` | El backend. Todo `static` salvo el vtable del final. |
| `shaders/malla.*` | Geometria con luz plana en 3 escalones. |
| `shaders/billboard.*` | El sprite que encara a la camara. |

## Dos decisiones que vale la pena entender

**Billboard sin rotacion.** El shader no rota nada: arma el cuadrado usando
el eje "derecha" y "arriba" de la camara como reglas. Si los lados del
cuadrado SON los ejes de la pantalla, el cuadrado es paralelo a la pantalla
por construccion. No puede no mirarte.

**Recorte por alfa (`discard`), no mezcla.** Mezclar obliga a ordenar los
sprites de atras hacia adelante cada frame. Con `discard` el pixel
transparente no escribe ni color ni profundidad, y el z-buffer resuelve el
orden solo. Se puede hacer porque el pixel art no tiene transparencias
suaves: un pixel esta o no esta. La limitacion del estilo habilita la
solucion simple.
