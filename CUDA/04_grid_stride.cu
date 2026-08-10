// ============================================================
// EJERCICIO 4 — COMPLETAR (el mas importante conceptualmente)
// Objetivo: GRID-STRIDE LOOP.
//
// Hasta ahora asumiste "1 thread = 1 elemento". Pero ¿que pasa
// si lanzas MENOS threads que elementos? (a veces conviene, para
// reusar threads y escribir kernels que escalan a cualquier N).
//
// La tecnica: cada thread procesa VARIOS elementos, saltando de a
// "stride" = total de threads del grid. Asi un grid chico cubre un
// array gigante. Este patron lo vas a ver EN TODOS LADOS.
// ============================================================
#include <cstdio>
#include <cuda_runtime.h>

__global__ void addGridStride(const float *a, const float *b, float *c, int n) {
    int start  = blockIdx.x * blockDim.x + threadIdx.x;

    // TODO 1: el "stride" es la cantidad TOTAL de threads del grid.
    //         Pista: blocks * threads_por_block = gridDim.x * blockDim.x
    int stride = /* ??? */ 1;

    // TODO 2: bucle que arranca en 'start' y salta de a 'stride'
    //         hasta cubrir los n elementos.
    // for (int i = start; i < n; i += ???) {
    //     c[i] = a[i] + b[i];
    // }
}

int main() {
    const int N = 1 << 20;
    const size_t bytes = N * sizeof(float);

    float *h_a = (float*)malloc(bytes);
    float *h_b = (float*)malloc(bytes);
    float *h_c = (float*)malloc(bytes);
    for (int i = 0; i < N; ++i) { h_a[i] = 3.0f; h_b[i] = 4.0f; }

    float *d_a, *d_b, *d_c;
    cudaMalloc(&d_a, bytes);
    cudaMalloc(&d_b, bytes);
    cudaMalloc(&d_c, bytes);
    cudaMemcpy(d_a, h_a, bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, bytes, cudaMemcpyHostToDevice);

    // A PROPOSITO lanzamos POCOS threads: 64 blocks * 256 = 16384 threads,
    // muchos menos que el millon de elementos. El grid-stride los cubre igual.
    addGridStride<<<64, 256>>>(d_a, d_b, d_c, N);
    cudaDeviceSynchronize();

    cudaMemcpy(h_c, d_c, bytes, cudaMemcpyDeviceToHost);
    int errores = 0;
    for (int i = 0; i < N; ++i) if (h_c[i] != 7.0f) errores++;
    printf(errores == 0 ? "OK: grid-stride cubrio todo.\n" : "FALLO: %d errores\n", errores);

    cudaFree(d_a); cudaFree(d_b); cudaFree(d_c);
    free(h_a); free(h_b); free(h_c);
    return errores == 0 ? 0 : 1;
}
