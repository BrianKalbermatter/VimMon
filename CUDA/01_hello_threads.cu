// ============================================================
// EJERCICIO 1 — RESUELTO (leelo, no lo completes)
// Objetivo: ENTENDER el indice global de un thread.
// Este es EL concepto fundamental de CUDA. Si entendes esto,
// entendes el 80% del modelo de programacion.
// ============================================================
#include <cstdio>
#include <cuda_runtime.h>

__global__ void helloThreads() {
    // Cada thread vive dentro de un BLOCK, y los blocks dentro de un GRID.
    //   threadIdx.x -> posicion del thread DENTRO de su block
    //   blockIdx.x  -> posicion del block DENTRO del grid
    //   blockDim.x  -> cuantos threads tiene cada block
    //
    // El indice GLOBAL (unico para todo el grid) se calcula asi:
    int global = blockIdx.x * blockDim.x + threadIdx.x;

    printf("block %d | thread local %d | indice GLOBAL %d\n",
           blockIdx.x, threadIdx.x, global);
}

int main() {
    // Lanzamos 3 blocks de 4 threads cada uno = 12 threads en total.
    // Mira la salida: vas a ver indices globales del 0 al 11,
    // pero el ORDEN es impredecible (corren en paralelo, ¿se entiende?).
    helloThreads<<<3, 4>>>();

    // Sincronizamos para que el host espere a que la GPU termine de imprimir.
    cudaDeviceSynchronize();
    return 0;
}
