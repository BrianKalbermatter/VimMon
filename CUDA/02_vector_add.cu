// ============================================================
// EJERCICIO 2 — COMPLETAR (busca los TODO)
// Objetivo: sumar dos vectores C = A + B en la GPU.
// Aplicas el indice global que aprendiste en el ej. 1.
// ============================================================
#include <cstdio>
#include <cuda_runtime.h>

__global__ void vectorAdd(const float *a, const float *b, float *c, int n) {
    // TODO 1: calcula el indice global del thread (igual que en el ej. 1)
    int i = /* ??? */ 0;

    // TODO 2: ¿por que necesitamos este "if"? Pista: ¿que pasa si lanzamos
    //         mas threads que elementos tiene el vector?
    if (i < n) {
        // TODO 3: escribi la suma del elemento i
        // c[i] = ???
    }
}

int main() {
    const int N = 1 << 20;                 // ~1 millon de elementos
    const size_t bytes = N * sizeof(float);

    float *h_a = (float*)malloc(bytes);
    float *h_b = (float*)malloc(bytes);
    float *h_c = (float*)malloc(bytes);
    for (int i = 0; i < N; ++i) { h_a[i] = 1.0f; h_b[i] = 2.0f; }

    float *d_a, *d_b, *d_c;
    cudaMalloc(&d_a, bytes);
    cudaMalloc(&d_b, bytes);
    cudaMalloc(&d_c, bytes);

    cudaMemcpy(d_a, h_a, bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_b, h_b, bytes, cudaMemcpyHostToDevice);

    int threads = 256;
    // TODO 4: calcula cuantos blocks necesitas para cubrir N elementos
    //         con 'threads' threads por block. Pista: redondeo hacia arriba.
    int blocks = /* ??? */ 1;

    vectorAdd<<<blocks, threads>>>(d_a, d_b, d_c, N);
    cudaDeviceSynchronize();

    cudaMemcpy(h_c, d_c, bytes, cudaMemcpyDeviceToHost);

    int errores = 0;
    for (int i = 0; i < N; ++i) if (h_c[i] != 3.0f) errores++;
    printf(errores == 0 ? "OK: suma correcta.\n" : "FALLO: %d errores\n", errores);

    cudaFree(d_a); cudaFree(d_b); cudaFree(d_c);
    free(h_a); free(h_b); free(h_c);
    return errores == 0 ? 0 : 1;
}
