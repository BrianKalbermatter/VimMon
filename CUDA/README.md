# Ejercicios CUDA — modelo de threads

Ruta: `~/VimMon/CUDA`
GPU: NVIDIA GeForce RTX 3070 Ti · CUDA Toolkit 13.3 (Arch WSL2)

## La regla que NO te podes olvidar

El sistema tiene **GCC 16**, que `nvcc` 13.3 NO soporta. Siempre compilas con `g++-15`:

```bash
nvcc -ccbin g++-15 archivo.cu -o programa
```

El Makefile ya lo hace por vos.

## Orden de los ejercicios

| Archivo | Concepto | Estado |
|---------|----------|--------|
| `01_hello_threads.cu` | indice global: `blockIdx*blockDim + threadIdx` | RESUELTO (leelo) |
| `02_vector_add.cu` | C = A + B, guarda contra desborde, calculo de blocks | COMPLETAR |
| `03_saxpy.cu` | y = a*x + y (escalar + in-place) | COMPLETAR |
| `04_grid_stride.cu` | grid-stride loop: menos threads que datos | COMPLETAR |

> Concepto siguiente: ver `05_geometria_precalculada.md` — precalcular
> geometría y solo buscar memoria (space-time tradeoff). El puente CUDA→Vulkan.

Hacelos EN ORDEN. Cada uno construye sobre el anterior.

## Como trabajar

1. Lee y compila el `01` para ver el modelo de threads en accion:
   ```bash
   make 01_hello_threads && ./01_hello_threads
   ```
2. Abri el `02`, busca los `// TODO`, completalos.
3. Compila SOLO ese: `make 02_vector_add && ./02_vector_add`
4. Si dice `OK` -> pasaste. Si no, revisa los TODO.
5. Repeti con `03` y `04`.

## Comandos utiles

```bash
make 02_vector_add     # compila un ejercicio puntual
make                   # compila todos
make run               # compila y ejecuta todos en orden
make clean             # borra los binarios
nvidia-smi             # ver la GPU
```

## Conceptos clave (para fijar)

- **thread**: la unidad minima de ejecucion. Corre el kernel una vez.
- **block**: grupo de threads que pueden cooperar (memoria compartida, sincronizacion).
- **grid**: todos los blocks de un lanzamiento `<<<blocks, threads>>>`.
- **indice global** = `blockIdx.x * blockDim.x + threadIdx.x` — memorizalo.
- **`if (i < n)`**: porque casi siempre lanzas MAS threads que datos (redondeo de blocks).
- **grid-stride**: cuando lanzas MENOS threads que datos, cada uno procesa varios.

> Primero el MODELO, despues la optimizacion. Memoria compartida, warps y
> coalescing vienen DESPUES de que esto te salga dormido.
