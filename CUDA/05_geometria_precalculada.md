# Concepto — Precalcular geometría, no recalcularla (space-time tradeoff)

> Esto NO es un ejercicio con código resuelto. Es el NORTE conceptual.
> Vos implementás. Acá está el mapa para que sepas QUÉ y POR QUÉ.

## La idea madre: space-time tradeoff

Hay dos recursos que podés cambiar uno por el otro:

- **CÓMPUTO** — hacer la cuenta cada vez (caro si se repite).
- **MEMORIA** — guardar el resultado y solo BUSCARLO (lookup).

> Regla: si calcular es caro y la entrada se repite → calculá UNA vez,
> guardá, y después solo direccioná memoria. "Calcular solo memorias".

Formas con nombre propio (buscalas y estudiálas):
- **Lookup Table (LUT)**: precalculás `sin(x)` de 0 a 360 en un array → `tabla[x]`.
- **Memoization / caching**: guardás resultados de funciones puras.
- **Vertex Buffer (VBO)**: los vértices de un objeto viven UNA vez en memoria.

## El matiz que te salva (NO todo se precalcula)

| Qué cosa                         | Tratamiento            | Por qué                          |
|----------------------------------|------------------------|----------------------------------|
| Geometría base (mesh, vértices)  | PRECALCULADA en memoria| Es invariante, nunca cambia      |
| Transformación (mover/rotar/escalar) | CÓMPUTO en runtime | Depende de tiempo/input → infinita |

No podés guardar "todas las posiciones posibles" de un personaje: explotás
la memoria. Guardás lo INVARIANTE, calculás lo VARIABLE.

## Por qué esto ES la filosofía de la GPU

La GPU existe para aplicar la MISMA matriz de transformación a miles de
vértices EN PARALELO. La geometría queda quieta en memoria (VBO); la GPU
solo aplica la matriz. Invariante en memoria + variable en cómputo paralelo.

Ese es el puente CUDA → Vulkan:
- Hoy (CUDA): aprendés el modelo de threads aplicando matemática paralela.
- Mañana (Vulkan): esa misma matemática paralela transforma vértices reales.

## Hacia dónde va esto en VimMon

- PAED define los objetos → eso es la geometría a GUARDAR.
- El renderer aplica transformaciones → eso es el CÓMPUTO (GPU después).

## TODO — para que lo implementes vos (en orden)

- [ ] Definir una struct para un vértice (x, y, z) en C.
- [ ] Guardar los 8 vértices de un cubo UNA vez en un array (la "geometría base").
- [ ] Escribir un kernel CUDA que aplique una traslación (sumar un offset) a
      los 8 vértices en paralelo — un thread por vértice.
- [ ] Medir: comparar recalcular cada frame vs leer de buffer + transformar.
- [ ] (Más adelante) Una LUT de seno/coseno para la matriz de rotación.

> Primero el MODELO (qué se guarda, qué se calcula). Después la optimización.
